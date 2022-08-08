#include "container.h"

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include "namespace.h"
#include "mount.h"
#include "user.h"
#include "log.h"
#include "oci.h"
#include "queue.h"
#include "clone.h"

typedef enum {
    CONTAINER_CREATING     = 0,
    CONTAINER_CREATED      = 1,
    CONTAINER_RUNNING      = 2,
    CONTAINER_STOPPED      = 3,
} conty_container_status_t;

#define __CONTAINER_STATUS_SIZE (CONTAINER_STOPPED + 1)

static const char *conty_container_statuses[__CONTAINER_STATUS_SIZE] = {
        [CONTAINER_CREATING] = "creating",
        [CONTAINER_CREATED]  = "created",
        [CONTAINER_RUNNING]  = "running",
        [CONTAINER_STOPPED]  = "stopped",
};

#define __CONTAINER_UID_BUF_SIZE 4096

struct conty_container {
    const char               *cc_id;
    conty_container_status_t  cc_status;

    struct {
        unsigned long cc_ns_new;
        int           cc_ns_fds[CONTY_NS_SIZE];
        char          cc_ns_has_fds;
    };

    struct conty_rootfs cc_rfs;

    struct {
        char                     cc_usr_ubuf[__CONTAINER_UID_BUF_SIZE];
        struct conty_user_id_map cc_usr_uidmap;

        char                     cc_usr_gbuf[__CONTAINER_UID_BUF_SIZE];
        struct conty_user_id_map cc_usr_gidmap;
    };

    struct {
        pid_t cc_pid;
        int   cc_pfd;
        int   cc_syncfds[2];
    };

    const char *cc_uts_hostname;
};

int conty_container_ns_init(struct conty_container *cc, const struct oci_conf *conf)
{
    int ns, fd;
    unsigned long ns_set = 0, flag;
    struct oci_namespace *ons_cur, *ons_tmp;

    cc->cc_ns_has_fds = 0;
    memset(cc->cc_ns_fds, -EBADF, CONTY_NS_SIZE * sizeof(int));

    LIST_FOREACH_SAFE(ons_cur, &conf->oc_namespaces, ons_next, ons_tmp) {
        ns = conty_ns_from_str(ons_cur->ons_type);
        if (ns < 0)
            return LOG_ERROR_RET(-1, "conty_container: unsupported namespace");

        flag = conty_ns_flags[ns];
        if (ns_set & flag) {
            LOG_WARN("conty_container: ignoring duplicate %s namespace",
                     ons_cur->ons_type);
            continue;
        }

        if (!ons_cur->ons_path)
            cc->cc_ns_new |= flag;
        else {
            fd = open(ons_cur->ons_path, O_RDONLY | O_CLOEXEC);
            if (fd < 0)
                return LOG_ERROR_RET(-1, "conty_container: cannot open namespace");

            cc->cc_ns_fds[ns] = fd;
            cc->cc_ns_has_fds = 1;
        }

        ns_set |= flag;
    }

    return 0;
}

int conty_container_ids_fill(struct conty_user_id_map *map,
                             const struct oci_uids *uids)
{
    int err = 0;
    struct oci_id_mapping *cur, *tmp;

    LIST_FOREACH_SAFE(cur, uids, oid_next, tmp) {
        err = conty_user_id_map_put(map, cur->oid_container,
                                    cur->oid_host, cur->oid_count);
        if (err != 0) {
            return LOG_ERROR_RET(err, "conty_container: cannot write uid mapping: %s",
                                 strerror(-err));
        }
    }

    return err;
}

int conty_container_user_maps_init(struct conty_container *cc,
                                   const struct oci_conf *conf)
{
    int err;

    conty_user_id_map_init(&cc->cc_usr_uidmap, cc->cc_usr_ubuf,
                           __CONTAINER_UID_BUF_SIZE);

    err = conty_container_ids_fill(&cc->cc_usr_uidmap, &conf->oc_uids);
    if (err != 0)
        return err;

    conty_user_id_map_init(&cc->cc_usr_gidmap, cc->cc_usr_gbuf,
                           __CONTAINER_UID_BUF_SIZE);

    return conty_container_ids_fill(&cc->cc_usr_gidmap, &conf->oc_gids);
}

int conty_container_runtime(void *cnt)
{
    int err;
    struct conty_container *cc = (struct conty_container *) cnt;

    if (cc->cc_ns_new & CLONE_NEWUSER) {
        err = conty_user_write_own_uid_mappings(&cc->cc_usr_uidmap);
        if (err != 0)
            return -1;

        err = conty_user_disable_own_setgroups();
        if (err != 0)
            return -1;

        err = conty_user_write_own_gid_mappings(&cc->cc_usr_gidmap);
        if (err != 0)
            return -1;
    }

    if (cc->cc_ns_new & CLONE_NEWUTS) {
        if (sethostname(cc->cc_uts_hostname, strlen(cc->cc_uts_hostname)) != 0)
            return -1;
    }

    if (cc->cc_ns_new & CLONE_NEWNS) {
        err = conty_rootfs_init_container(&cc->cc_rfs);
        if (err != 0)
            return -1;

        err = conty_rootfs_mount(&cc->cc_rfs);
        if (err != 0)
            return -1;

       /* err = conty_rootfs_mount_devices(&cc->cc_rfs);
        if (err != 0)
            return -1;*/

        if (cc->cc_ns_new & CLONE_NEWPID) {
            err = conty_rootfs_mount_proc(&cc->cc_rfs);
            if (err != 0)
                return -1;
        }

        if (cc->cc_ns_new & CLONE_NEWNET) {
            err = conty_rootfs_mount_sys(&cc->cc_rfs);
            if (err != 0)
                return -1;
        }
    }

    return 0;
}

int conty_container_joiner(void *cnt)
{
    int nsfd;
    struct conty_container *cc = (struct conty_container *) cnt;

    for (conty_ns_t ns = 0; ns < CONTY_NS_SIZE; ns++) {
        nsfd = cc->cc_ns_fds[ns];
        if (nsfd < 0)
            continue;

        if (conty_ns_set(nsfd, 0) < 0) {
            return LOG_ERROR_RET(-1, "conty_container: cannot join %s namespace",
                                 conty_ns_names[ns]);
        }
    }

    cc->cc_pid = conty_clone3_cb(conty_container_runtime, cnt,
                                 cc->cc_ns_new | CLONE_PARENT | CLONE_PIDFD,
                                 &cc->cc_pfd);

    if (cc->cc_pfd < 0)
        return LOG_ERROR_RET(-1, "conty_container: cannot spawn runtime process");

    return 0;
}

int conty_container_spawn_joiner(struct conty_container *container)
{
    pid_t joiner_pid;
    int joiner_status;

    joiner_pid = conty_clone(conty_container_joiner, container,
                             CLONE_VFORK | CLONE_VM | CLONE_FILES, NULL);
    if (joiner_pid < 0)
        return LOG_ERROR_RET(-errno, "conty_container: cannot spawn joiner process");

    if (waitpid(joiner_pid, &joiner_status, 0) != joiner_pid)
        return LOG_ERROR_RET(-errno, "conty_container: cannot await joiner process");

    if (!WIFEXITED(joiner_status) || WEXITSTATUS(joiner_status) != 0)
        return LOG_ERROR_RET(-errno, "conty_container: joiner process failed");

    return 0;
}

struct conty_container *conty_container_create(const char *cc_id,
                                               const struct oci_conf *conf)
{
    int err;
    struct conty_container *cc = NULL;

    if (!cc_id)
        return LOG_ERROR_RET(NULL, "conty_container: invalid container id");

    cc = calloc(1, sizeof(struct conty_container));
    if (!cc)
        return LOG_FATAL_RET(NULL, "conty_container: out of memory");

    cc->cc_id = cc_id;

    err = conty_container_ns_init(cc, conf);
    if (err != 0)
        return NULL;

    const char *target = "/home/nas/temprfs";

    conty_rootfs_init_runtime(&cc->cc_rfs, conf->oc_rootfs.ocirfs_path,
                              target, conf->oc_rootfs.ocirfs_ro);

    err = conty_container_user_maps_init(cc, conf);
    if (err != 0)
        return NULL;

    cc->cc_uts_hostname = conf->oc_hostname;

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, cc->cc_syncfds) != 0)
        return LOG_ERROR_RET(NULL, "conty_container: cannot setup socketpair");

    if (cc->cc_ns_has_fds) {
        if (conty_container_spawn_joiner(cc) != 0)
            return NULL;
    } else {
        cc->cc_pid = conty_clone3_cb(conty_container_runtime, cc,
                                     cc->cc_ns_new, &cc->cc_pfd);
        if (cc->cc_pid < 0)
            return LOG_ERROR_RET(NULL, "conty_container: cannot spawn runtime process");
    }

    return NULL;
}