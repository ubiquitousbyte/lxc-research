#include "container.h"

#include <fcntl.h>
#include <sys/socket.h>

#include "log.h"
#include "clone.h"
#include "resource.h"
#include "syscall.h"
#include "sync.h"

static int conty_container_run(void *container);
static int conty_container_join(void *cnt);
static int conty_container_spawn(struct conty_container *cc);
static int conty_container_init_namespaces_parent(struct conty_container *cc);
static int conty_container_exec_hooks(const struct conty_container *cc, int event);

struct conty_container *conty_container_new(const char *cc_id, const char *path)
{
    int err;
    CONTY_INVOKE_CLEANER(conty_container_free) struct conty_container *cc = NULL;
    struct oci_conf *conf = NULL;

    cc = malloc(sizeof(struct conty_container));
    if (!cc)
        return LOG_ERROR_RET(NULL, "out of memory");

    if (!(conf = oci_deser_conf_file(path)))
        return NULL;

    cc->cc_syncfds[CONTY_SYNC_CFD]  = -EBADF;
    cc->cc_syncfds[CONTY_SYNC_RTFD] = -EBADF;
    cc->cc_pollfd                   = -EBADF;
    cc->cc_id                       =  cc_id;
    cc->cc_oci_status               =  CONTAINER_CREATING;
    cc->cc_oci                      =  CONTY_MOVE_PTR(conf);

    if (conty_container_init_namespaces_parent(cc) != 0)
        return NULL;

    if (conty_sync_init(cc->cc_syncfds) != 0)
        return NULL;

    if (conty_container_spawn(cc) != 0)
        return NULL;

    conty_sync_parent_init(cc->cc_syncfds);

    err = conty_sync_parent_wait(cc->cc_syncfds, SYNC_RUNTIME_CREATE);
    if (err != 0)
        goto reap_and_exit;

    err = conty_container_exec_hooks(cc, SYNC_RUNTIME_CREATED);
    if (err != 0)
        goto notify_err;

    err = conty_sync_parent_tx(cc->cc_syncfds, SYNC_CONTAINER_CREATE,
                               SYNC_CONTAINER_CREATED);
    if (err != 0)
        goto reap_and_exit;

    cc->cc_oci_status = CONTAINER_CREATED;

    return CONTY_MOVE_PTR(cc);

notify_err:
    conty_sync_parent_wake(cc->cc_syncfds, SYNC_ERROR);
reap_and_exit:
    conty_clone_wait_exited(cc->cc_pid, NULL);
    return NULL;
}

int conty_container_start(struct conty_container *cc)
{
    int err;
    if (cc->cc_oci_status != CONTAINER_CREATED)
        return -1;

    err = conty_sync_parent_tx(cc->cc_syncfds, SYNC_CONTAINER_START,
                               SYNC_CONTAINER_STARTED);
    if (err != 0)
        return err;

    cc->cc_oci_status = CONTAINER_RUNNING;

    return 0;
}

int conty_container_kill(struct conty_container *cc, int sig)
{
    if (conty_pidfd_send_signal(cc->cc_pollfd, sig, NULL, 0) != 0)
        return LOG_ERROR_RET(-errno, "cannot kill container %s", cc->cc_id);
    return 0;
}

static int conty_container_spawn(struct conty_container *cc)
{
    if (!cc->cc_ns_has_fds) {
        cc->cc_pid = conty_clone3_cb(conty_container_run, cc,
                                     cc->cc_ns_new, &cc->cc_pollfd);
        if (cc->cc_pid < 0)
            return LOG_ERROR_RET(-1, "cannot create runtime process");
    } else {
        pid_t pid;
        int status;

        pid = conty_clone(conty_container_join, cc,
                          CLONE_VFORK | CLONE_VM | CLONE_FILES, NULL);

        if (conty_clone_wait_exited(pid, &status) != 0)
            return LOG_ERROR_RET(-1, "cannot create runtime process");
    }

    return 0;
}

static int conty_container_run(void *container)
{
    CONTY_INVOKE_CLEANER(conty_container_free) struct conty_container *cc = NULL;
    int err;
    struct conty_rootfs rootfs;

    cc = (struct conty_container *) container;

    conty_sync_child_init(cc->cc_syncfds);

    close(cc->cc_pollfd);
    cc->cc_pollfd = -EBADF;

    if (conty_sync_child_wake(cc->cc_syncfds, SYNC_RUNTIME_CREATE) != 0)
        return -1;

    if (cc->cc_ns_new & CLONE_NEWUSER) {
        if ((err = conty_oci_write_uid_map(&cc->cc_oci->oc_uids)) != 0)
            goto sync_err;

        if ((err = conty_usr_deny_setgroups()) != 0)
            goto sync_err;

        if ((err = conty_oci_write_gid_map(&cc->cc_oci->oc_gids)) != 0)
            goto sync_err;
    }

    if (cc->cc_ns_new & CLONE_NEWNS) {
        struct oci_rootfs *ocirfs = &cc->cc_oci->oc_rootfs;

        err = conty_rootfs_init(&rootfs, cc->cc_id,
                                ocirfs->ocirfs_path, ocirfs->ocirfs_ro);
        if (err != 0)
            goto sync_err;

        if ((err = conty_rootfs_mount(&rootfs)) != 0)
            goto sync_err;

        if ((err = conty_rootfs_mount_devfs(&rootfs)) != 0)
            goto sync_err;

        if ((err = conty_rootfs_mkdevices(&rootfs)) != 0)
            goto sync_err;

        if (cc->cc_ns_new & CLONE_NEWPID) {
            if ((err = conty_rootfs_mount_procfs(&rootfs)) != 0)
                goto sync_err;
        }

        if (cc->cc_ns_new & CLONE_NEWNET) {
            if ((err = conty_rootfs_mount_sysfs(&rootfs)) != 0)
                goto sync_err;
        }
    }

    if (conty_sync_child_wait(cc->cc_syncfds, SYNC_CONTAINER_CREATE) != 0)
        return -1;

    if ((err = conty_container_exec_hooks(cc, SYNC_CONTAINER_CREATED)) != 0)
        goto sync_err;

    if (cc->cc_ns_new & CLONE_NEWNS) {
        if ((err = conty_rootfs_pivot(&rootfs)) != 0)
            goto sync_err;
    }

    err = conty_sync_child_tx(cc->cc_syncfds, SYNC_CONTAINER_CREATED,
                              SYNC_CONTAINER_START);
    if (err != 0)
        return -1;

    if ((err = conty_container_exec_hooks(cc, SYNC_CONTAINER_START)) != 0)
        goto sync_err;

    // TODO: Execute process

    err = conty_sync_child_wake(cc->cc_syncfds, SYNC_CONTAINER_STARTED);
    if (err != 0)
        return -1;

    return 0;
sync_err:
    conty_sync_child_wake(cc->cc_syncfds, SYNC_ERROR);
    return err;
}

static int conty_container_join(void *cnt)
{
    int nsfd;
    struct conty_container *cc = (struct conty_container *) cnt;

    for (conty_ns_t ns = 0; ns < CONTY_NS_SIZE; ns++) {
        nsfd = cc->cc_ns_fds[ns];
        if (nsfd < 0)
            continue;

        if (conty_ns_set(nsfd, 0) < 0)
            return LOG_ERROR_RET(-1, "cannot join namespace");
    }

    cc->cc_pid = conty_clone3_cb(conty_container_run, cnt,
                                 cc->cc_ns_new | CLONE_PARENT | CLONE_PIDFD,
                                 &cc->cc_pollfd);

    if (cc->cc_pid < 0)
        return LOG_ERROR_RET(-1, "cannot spawn runtime");

    return 0;
}

static int conty_container_exec_hooks(const struct conty_container *cc, int event)
{
    const struct oci_hooks table[CONTY_SYNC_MAX] = {
            [SYNC_RUNTIME_CREATED]   = cc->cc_oci->oc_hooks.oeh_on_runtime_create,
            [SYNC_CONTAINER_CREATED] = cc->cc_oci->oc_hooks.oeh_on_container_created,
            [SYNC_CONTAINER_START]   = cc->cc_oci->oc_hooks.oeh_on_container_start,
            [SYNC_CONTAINER_STARTED] = cc->cc_oci->oc_hooks.oeh_on_container_start,
    };
    struct oci_process_state state;

    state.oprocst_container_pid = cc->cc_pid;
    state.oprocst_container_id  = cc->cc_id;
    state.oprocst_status        = oci_container_statuses[cc->cc_oci_status];
    state.oprocst_rootfs        = cc->cc_oci->oc_rootfs.ocirfs_path;

    struct oci_hook *cur, *tmp;
    LIST_FOREACH_SAFE(cur, &table[event], oh_next, tmp) {
        if (conty_oci_hook_exec(cur, &state) != 0)
            return LOG_ERROR_RET(-1, "event hook failed");
    }

    return 0;
}

static int conty_container_init_namespaces_parent(struct conty_container *cc)
{
    int ns, fd;
    unsigned long ns_set = 0, flag;
    struct oci_namespace *ons_cur, *ons_tmp;
    struct oci_namespaces *namespaces = &cc->cc_oci->oc_namespaces;

    cc->cc_ns_has_fds = 0;
    memset(cc->cc_ns_fds, -EBADF, CONTY_NS_SIZE * sizeof(int));

    LIST_FOREACH_SAFE(ons_cur, namespaces, ons_next, ons_tmp) {
        ns = conty_ns_from_str(ons_cur->ons_type);
        if (ns < 0)
            return LOG_ERROR_RET(-1, "unsupported namespace");

        flag = conty_ns_flags[ns];
        if (ns_set & flag) {
            LOG_WARN("ignoring duplicate %s namespace", ons_cur->ons_type);
            continue;
        }

        if (!ons_cur->ons_path)
            cc->cc_ns_new |= flag;
        else {
            fd = open(ons_cur->ons_path, O_RDONLY | O_CLOEXEC);
            if (fd < 0)
                return LOG_ERROR_RET(-1, "cannot open namespace");

            cc->cc_ns_fds[ns] = fd;
            cc->cc_ns_has_fds = 1;
        }

        ns_set |= flag;
    }

    return 0;
}

void conty_container_free(struct conty_container *cc)
{
    if (cc) {
        if (cc->cc_oci)
            oci_conf_free(cc->cc_oci);
        if (cc->cc_syncfds[0] >= 0)
            close(cc->cc_syncfds[0]);
        if (cc->cc_syncfds[1] >= 0)
            close(cc->cc_syncfds[1]);
        if (cc->cc_pollfd >= 0)
            close(cc->cc_pollfd);
        for (conty_ns_t i = 0; i < CONTY_NS_SIZE; i++) {
            if (cc->cc_ns_fds[i] >= 0)
                close(cc->cc_ns_fds[i]);
        }
        free(cc);
    }
}