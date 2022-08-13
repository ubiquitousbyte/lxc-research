#ifndef CONTY_NAMESPACE_H
#define CONTY_NAMESPACE_H

#include <errno.h>
#include <string.h>
#include <sched.h>

typedef enum conty_ns_t {
    CONTY_NS_USER = 0,
    CONTY_NS_PID  = 1,
    CONTY_NS_MNT  = 2,
    CONTY_NS_NET  = 3,
    CONTY_NS_UTS  = 4,
    CONTY_NS_IPC  = 5,
} conty_ns_t;

#define CONTY_NS_STRLEN (sizeof("cgroup") - 1)

#define CONTY_NS_LEN (CONTY_NS_IPC + 1)

static const char *conty_ns_str[CONTY_NS_STRLEN] = {
        [CONTY_NS_USER] = "user",
        [CONTY_NS_PID]  = "pid",
        [CONTY_NS_MNT]  = "mnt",
        [CONTY_NS_NET]  = "net",
        [CONTY_NS_UTS]  = "uts",
        [CONTY_NS_IPC]  = "ipc",
};

static const int conty_ns_flags[CONTY_NS_LEN] = {
        [CONTY_NS_USER] = CLONE_NEWUSER,
        [CONTY_NS_PID]  = CLONE_NEWPID,
        [CONTY_NS_MNT]  = CLONE_NEWNS,
        [CONTY_NS_NET]  = CLONE_NEWNET,
        [CONTY_NS_UTS]  = CLONE_NEWUTS,
        [CONTY_NS_IPC]  = CLONE_NEWIPC,
};

static inline int conty_ns_from_str(const char *str)
{
    for (conty_ns_t ns = 0; ns < CONTY_NS_LEN; ns++) {
        if (strncmp(str, conty_ns_str[ns], CONTY_NS_STRLEN) == 0)
            return ns;
    }
    return -ENOENT;
}

static inline int conty_ns_set(int fd, int type)
{
    return (setns(fd, (!type) ? 0 : conty_ns_flags[type]) < 0) ? -errno : 0;
}

#endif //CONTY_NAMESPACE_H
