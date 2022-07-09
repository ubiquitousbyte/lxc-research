#define _GNU_SOURCE

#include "namespace.h"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sched.h>
#include <sys/stat.h>
#include <fcntl.h>

struct conty_ns {
    /*
     * A file descriptor referencing the namespace object in procfs
     * Even if all processes inside the namespace terminate, the namespace
     * will be kept alive until this file descriptor is released
     */
    int           fd;
    /*
     * The inode number and the device identifier of the proc file uniquely
     * identify the namespace
     */
    ino_t         ino;
    dev_t         dev;

    conty_ns_type type;
};

const static char *const conty_ns_name_table[CONTY_NS_MAX] = {
        [CONTY_NS_CGROUP] = "cgroup",
        [CONTY_NS_IPC]    = "ipc",
        [CONTY_NS_MOUNT]  = "mount",
        [CONTY_NS_NET]    = "net",
        [CONTY_NS_PID]    = "pid",
        [CONTY_NS_USER]   = "user",
        [CONTY_NS_UTS]    = "uts"
};

const static int conty_ns_flag_table[CONTY_NS_MAX] = {
        [CONTY_NS_CGROUP] = CLONE_NEWCGROUP,
        [CONTY_NS_IPC]    = CLONE_NEWIPC,
        [CONTY_NS_MOUNT]  = CLONE_NEWNS,
        [CONTY_NS_NET]    = CLONE_NEWNET,
        [CONTY_NS_PID]    = CLONE_NEWPID,
        [CONTY_NS_USER]   = CLONE_NEWUSER,
        [CONTY_NS_UTS]    = CLONE_NEWUTS
};

struct conty_ns *conty_ns_open(pid_t pid, conty_ns_type type)
{
    if (pid < 1) {
        errno = ESRCH;
        return NULL;
    }

    char path[sizeof("/proc/xxxxxxxxxxxxxxxxxxxx/ns/cgroup")];
    const char *ns_name = conty_ns_name_table[type];

    snprintf(path, sizeof(path), "/proc/%d/ns/%s", pid, ns_name);

    /*
     * We open the file with the O_CLOEXEC file to ensure that the descriptor
     * is closed if the runtime or any of its children calls exec
     */
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return NULL;

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

struct conty_ns *conty_ns_open_current(conty_ns_type type)
{
    return conty_ns_open(getpid(), type);
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
    int rc = setns(ns->fd, conty_ns_flag_table[ns->type]);
    if (rc != 0)
        rc = -errno;
    return rc;
}

int conty_ns_detach(int flags)
{
    /*
     * We switch off any bits in the mask unrelated to namespace configurations
     */
    int rc = unshare(flags & ~(CLONE_FS | CLONE_FILES | CLONE_SYSVSEM));
    if (rc != 0)
        rc = -errno;
    return rc;
}

void conty_ns_close(struct conty_ns *ns)
{
    if (ns) {
        close(ns->fd);
        free(ns);
    }
}