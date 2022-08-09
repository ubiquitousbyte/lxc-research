#include "namespace.h"

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/nsfs.h>

#include "resource.h"

#define CONTY_NS_PATH_MAX (sizeof("/proc/xxxxxxxxxxxxxxxxxxxx/ns/cgroup") - 1)

int conty_ns_open(conty_ns_t ns, pid_t pid)
{
    int nsfd;
    char path[CONTY_NS_PATH_MAX];

    int err = CONTY_SNPRINTF(path, sizeof(path),
                             "/proc/%d/ns/%s", pid, conty_ns_names[ns]);
    if (err < 0)
        return err;

    nsfd = open(path, O_RDONLY | O_CLOEXEC);
    if (nsfd < 0)
        return -errno;

    return nsfd;
}

int conty_ns_unshare(int flags)
{
    if (flags & (CLONE_FS | CLONE_FILES | CLONE_SYSVSEM |
                 CLONE_THREAD | CLONE_SIGHAND | CLONE_VM)) {
        errno = EINVAL;
        return -1;
    }
    return unshare(flags);
}

int conty_ns_set(int fd, int type)
{
    return setns(fd, (type == 0) ? 0 : conty_ns_flags[type]);
}

int conty_ns_parent(int fd)
{
    return ioctl(fd, NS_GET_PARENT);
}