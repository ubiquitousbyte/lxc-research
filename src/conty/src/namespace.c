#define _GNU_SOURCE

#include "namespace.h"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/nsfs.h>

struct conty_ns {
    /*
     * A file descriptor referencing the namespace object in procfs
     * Even if all processes inside the namespace terminate, the namespace
     * will be kept alive until this file descriptor is released
     */
    int           fd;
    /*
     * The inode number and the device identifier of the proc file uniquely
     * identifying the namespace
     */
    ino_t         ino;
    dev_t         dev;
    /*
     * The flag identifying the namespace type
     */
    int           type;
};

/*
 * The caller must ensure that fd is a descriptor pointing
 * to an actual namespace object
 */
static struct conty_ns *__conty_ns_from_fd(int fd, int type)
{
    /* Fetch the inode number and device id from the kernel */
    struct stat metadata;
    if (fstat(fd, &metadata) == -1)
        return NULL;

    struct conty_ns *namespace = calloc(1, sizeof(struct conty_ns));
    if (!namespace)
        return NULL;

    namespace->fd = fd;
    namespace->ino = metadata.st_ino;
    namespace->dev = metadata.st_dev;
    namespace->type = type;

    return namespace;
}

static const char *__conty_ns_flag_to_name(int flag)
{
    switch (flag) {
    case CLONE_NEWUSER:
        return "user";
    case CLONE_NEWPID:
        return "pid";
    case CLONE_NEWUTS:
        return "uts";
    case CLONE_NEWNET:
        return "net";
    case CLONE_NEWIPC:
        return "ipc";
    case CLONE_NEWCGROUP:
        return "cgroup";
    case CLONE_NEWNS:
        return "mnt";
    default:
        return NULL;
    }
}

struct conty_ns *conty_ns_open(pid_t pid, int type)
{
    if (pid < 1) {
        errno = ESRCH;
        return NULL;
    }

    const char *ns_name = __conty_ns_flag_to_name(type);
    if (!ns_name) {
        errno = EINVAL;
        return NULL;
    }

    char path[sizeof("/proc/xxxxxxxxxxxxxxxxxxxx/ns/cgroup")];
    snprintf(path, sizeof(path), "/proc/%d/ns/%s", pid, ns_name);

    /*
     * We open the file with the O_CLOEXEC file to ensure that the descriptor
     * is closed if the runtime or any of its children calls exec
     */
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return NULL;

    struct conty_ns *namespace = __conty_ns_from_fd(fd, type);
    if (!namespace) {
        close(fd);
        return NULL;
    }

    return namespace;
}

struct conty_ns *conty_ns_open_current(int type)
{
    return conty_ns_open(getpid(), type);
}

struct conty_ns *conty_ns_from_fd(int fd)
{
    int ns_type = ioctl(fd, NS_GET_NSTYPE);
    if (ns_type < 0)
        return NULL;
    return __conty_ns_from_fd(fd, ns_type);
}

int conty_ns_type(const struct conty_ns *ns)
{
    return ns->type;
}

int conty_ns_is(const struct conty_ns *left, const struct conty_ns *right)
{
    return (left->ino == right->ino) && (left->dev == right->dev);
}

ino_t conty_ns_inode(const struct conty_ns *ns)
{
    return ns->ino;
}

dev_t conty_ns_device(const struct conty_ns *ns)
{
    return ns->dev;
}

int conty_ns_join(const struct conty_ns *ns)
{
    return (setns(ns->fd, ns->type) != 0) ? -errno : 0;
}

int conty_ns_detach(int flags)
{
    /*
     * Any bits in the mask unrelated to namespace configuration
     * effectively render the operation invalid
     */
    if (flags & (CLONE_FS | CLONE_FILES | CLONE_SYSVSEM | CLONE_VM |
                 CLONE_SETTLS | CLONE_CHILD_SETTID | CLONE_PARENT_SETTID))
        return -EINVAL;

    return (unshare(flags) != 0) ? -errno : 0;
}

struct conty_ns *conty_ns_parent(const struct conty_ns *ns)
{
    if (ns->type != CLONE_NEWUSER && ns->type != CLONE_NEWPID) {
        /*
         * Parent relationships exist solely in hierarchical namespaces.
         * As of now, only the user and pid namespaces are hierarchical,
         * so we check that here to avoid redundant ioctling
         */
        errno = EINVAL;
        return NULL;
    }

    int pfd = ioctl(ns->fd, NS_GET_PARENT);
    if (pfd < 0)
        return NULL;

    struct conty_ns *parent = __conty_ns_from_fd(pfd, ns->type);
    if (!parent) {
        close(pfd);
        return NULL;
    }

    return parent;
}

void conty_ns_close(struct conty_ns *ns)
{
    if (ns) {
        close(ns->fd);
        free(ns);
    }
}

int conty_ns_id_map_init(struct conty_ns_id_map *m, char *buf, size_t buf_size)
{
    if (buf_size > CONTY_NS_ID_MAP_MAX)
        return -ENOSPC;

    m->buf = buf;
    m->cap = buf_size;
    m->written = 0;

    return 0;
}

int conty_ns_id_map_put(struct conty_ns_id_map *m, unsigned left,
                        unsigned right, unsigned range)
{
    if (range < 1)
        return -EINVAL;

    size_t spc = snprintf(NULL, 0, "%u %u %u\n", left, right, range);
    if (m->written + spc > m->cap)
        return -ENOSPC;

    sprintf(&m->buf[m->written], "%u %u %u\n", left, right, range);
    m->written += spc;

    return 0;
}