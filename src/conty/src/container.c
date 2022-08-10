#include "container.h"

#include <fcntl.h>
#include <sys/socket.h>

#include "log.h"
#include "clone.h"
#include "resource.h"
#include "syscall.h"

#define CONTY_SYNC_RTFD 0
#define CONTY_SYNC_CFD  1

typedef enum {
    CONTY_SYNC_RT_CREATED,
    CONTY_SYNC_CONTAINER_CREATED,
} conty_sync_event_t ;

static int conty_container_sync_tx(int fd, conty_sync_event_t event)
{
    int e = event;
    if (write(fd, &e, sizeof(int)) != sizeof(int))
        return LOG_ERROR_RET(-1, "could not sync runtime and container");
    return 0;
}

static int conty_container_sync_rx(int fd, conty_sync_event_t event)
{
    int e;
    if (read(fd, &e, sizeof(int)) != sizeof(int))
        return LOG_ERROR_RET(-1, "could not sync runtime and container");
    if (e != event)
        return LOG_ERROR_RET(-1, "expected rx event %d but got %d", event, e);
    return 0;
}

static int conty_container_exec_hook(const struct conty_container *cc,
                                     const struct oci_hook *hook)
{
    int err;
    struct oci_process_state state;

    conty_container_get_state(cc, &state);
    if ((err = conty_oci_hook_exec(hook, &state)) != 0)
        return LOG_ERROR_RET(err, "event hook failed: %s", hook->oh_path);

    return err;
}

static int conty_container_init_ns(struct conty_container *cc)
{
    int ns, fd;
    unsigned long ns_set = 0, flag;
    struct oci_namespace *ons_cur, *ons_tmp;
    struct oci_namespaces *namespaces = &cc->cc_oci_conf->oc_namespaces;

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

static int conty_container_rootfs_init(struct conty_container *cc)
{
    int err;
    char cwd[PATH_MAX];
    char id[64];
    struct oci_rootfs *oci_rfs = &cc->cc_oci_conf->oc_rootfs;

    cc->cc_mnt_root.crfs_source = CONTY_MOVE_PTR(oci_rfs->ocirfs_path);
    if (!getcwd(cwd, PATH_MAX))
        return LOG_ERROR_RET(-errno, "cannot open cwd");

    err = CONTY_SNPRINTF(id, 64, "/%s", cc->cc_id);
    if (err < 0)
        return LOG_ERROR_RET(err, "cannot create path to rootfs target directory");

    strncat(cwd, id, 64);
    cc->cc_mnt_root.crfs_target = strndup(cwd, PATH_MAX);

    return 0;
}

static int conty_container_spawner(void *cnt)
{
    int err;
    CONTY_INVOKE_CLEANER(conty_container_free) struct conty_container *cc = NULL;
    struct oci_conf *conf = NULL;
    __CONTY_CLOSE int sync_fd = -EBADF;
    struct oci_hook *cur, *tmp;

    cc         = (struct conty_container *) cnt;
    cc->cc_pid = conty_getpid();
    conf       = cc->cc_oci_conf;

    sync_fd = CONTY_MOVE_FD(cc->cc_syncfds[CONTY_SYNC_CFD]);
    close(cc->cc_syncfds[CONTY_SYNC_RTFD]);

    if (conty_container_sync_tx(sync_fd, CONTY_SYNC_RT_CREATED) != 0)
        return -1;

    if (cc->cc_ns_new & CLONE_NEWUSER) {
        if ((err = conty_oci_write_uid_map(&conf->oc_uids)) != 0)
            return LOG_ERROR_RET(err, "cannot write uid mappings");

        if ((err = conty_usr_deny_setgroups()) != 0)
            return LOG_ERROR_RET(err, "cannot deny setgroups");

        if ((err = conty_oci_write_gid_map(&conf->oc_gids)) != 0)
            return LOG_ERROR_RET(err, "cannot write gid mappings");
    }

    if (cc->cc_ns_new & CLONE_NEWNS) {
        if ((err = conty_rootfs_mount(&cc->cc_mnt_root)) != 0)
            return err;

        if ((err = conty_rootfs_mount_devfs(&cc->cc_mnt_root)) != 0)
            return err;

        if ((err = conty_rootfs_mkdevices(&cc->cc_mnt_root)) != 0)
            return err;

        if (cc->cc_ns_new & CLONE_NEWPID) {
            if ((err = conty_rootfs_mount_procfs(&cc->cc_mnt_root)) != 0)
                return err;
        }

        if (cc->cc_ns_new & CLONE_NEWNET) {
            if ((err = conty_rootfs_mount_sysfs(&cc->cc_mnt_root)) != 0)
                return err;
        }
    }

    if (cc->cc_ns_new & CLONE_NEWUTS) {
        if (sethostname(conf->oc_hostname, strlen(conf->oc_hostname)) != 0)
            return -1;
    }

    if (conty_container_sync_rx(sync_fd, CONTY_SYNC_CONTAINER_CREATED) != 0)
        return -1;

    LIST_FOREACH_SAFE(cur, &conf->oc_hooks.oeh_sb_created, oh_next, tmp) {
        if (conty_container_exec_hook(cc, cur) != 0)
            return -1;
    }

    conty_rootfs_pivot(&cc->cc_mnt_root);

    return 0;
}

static int conty_container_joiner(void *cnt)
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

    cc->cc_pid = conty_clone3_cb(conty_container_spawner, cnt,
                                 cc->cc_ns_new | CLONE_PARENT | CLONE_PIDFD,
                                 &cc->cc_pollfd);

    if (cc->cc_pid < 0)
        return LOG_ERROR_RET(-1, "cannot spawn runtime");

    return 0;
}

struct conty_container* conty_container_new(const char *cc_id, const char *path)
{
    CONTY_INVOKE_CLEANER(conty_container_free) struct conty_container *cc = NULL;
    CONTY_INVOKE_CLEANER(oci_conf_free) struct oci_conf *conf = NULL;
    __CONTY_CLOSE int sync_fd = -EBADF;
    struct oci_hook *cur, *tmp;

    conf = oci_deser_conf_file(path);
    if (!conf)
        return NULL;

    cc = calloc(1, sizeof(struct conty_container));
    if (!cc)
        return LOG_ERROR_RET_ERRNO(NULL, errno, "out of memory");

    cc->cc_syncfds[CONTY_SYNC_CFD]  = -EBADF;
    cc->cc_syncfds[CONTY_SYNC_RTFD] = -EBADF;
    cc->cc_pollfd                   = -EBADF;
    cc->cc_id                       =  cc_id;
    cc->cc_oci_status               =  CONTAINER_CREATING;
    cc->cc_oci_conf                 =  CONTY_MOVE_PTR(conf);

    if (conty_container_init_ns(cc) != 0)
        return NULL;

    if (conty_container_rootfs_init(cc) != 0)
        return NULL;

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, cc->cc_syncfds) != 0)
        return LOG_ERROR_RET(NULL, "cannot create ipc socket pair");

    sync_fd = CONTY_MOVE_FD(cc->cc_syncfds[CONTY_SYNC_RTFD]);
    close(cc->cc_syncfds[CONTY_SYNC_CFD]);

    if (cc->cc_ns_has_fds) {
        pid_t joiner_pid;
        int joiner_status;

        joiner_pid = conty_clone(conty_container_joiner, cc,
                                 CLONE_VFORK | CLONE_VM | CLONE_FILES, NULL);

        if (conty_clone_wait_exited(joiner_pid, &joiner_status) != 0)
            return LOG_ERROR_RET(NULL, "cannot create runtime process");
    } else {
        cc->cc_pid = conty_clone3_cb(conty_container_spawner, cc,
                                     cc->cc_ns_new, &cc->cc_pollfd);
        if (cc->cc_pid < 0)
            return LOG_ERROR_RET(NULL, "cannot create runtime process");
    }

    if (conty_container_sync_rx(sync_fd, CONTY_SYNC_RT_CREATED) != 0)
        return NULL;

    LIST_FOREACH_SAFE(cur, &conf->oc_hooks.oeh_rt_create, oh_next, tmp) {
        if (conty_container_exec_hook(cc, cur) != 0)
            return NULL;
    }

    if (conty_container_sync_tx(sync_fd, CONTY_SYNC_CONTAINER_CREATED) != 0)
        return NULL;

    return CONTY_MOVE_PTR(cc);
}

void conty_container_get_state(const struct conty_container *cc,
                               struct oci_process_state *state)
{
    state->oprocst_status = oci_container_statuses[cc->cc_oci_status];
    state->oprocst_rootfs = cc->cc_mnt_root.crfs_target;
    state->oprocst_sbid   = cc->cc_id;
    state->oprocst_sbpid  = cc->cc_pid;
}

void conty_container_free(struct conty_container *cc)
{
    if (cc) {
        if (cc->cc_syncfds[CONTY_SYNC_CFD] >= 0)
            close(cc->cc_syncfds[CONTY_SYNC_CFD]);
        if (cc->cc_syncfds[CONTY_SYNC_RTFD] >= 0)
            close(cc->cc_syncfds[CONTY_SYNC_RTFD]);
        if (cc->cc_pollfd >= 0)
            close(cc->cc_pollfd);
        if (cc->cc_oci_conf)
            oci_conf_free(cc->cc_oci_conf);
        if (cc->cc_mnt_root.crfs_target)
            free(cc->cc_mnt_root.crfs_target);
        if (cc->cc_mnt_root.crfs_source)
            free(cc->cc_mnt_root.crfs_source);
        free(cc);
    }
}