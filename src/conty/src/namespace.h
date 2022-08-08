#ifndef CONTY_NAMESPACE_H
#define CONTY_NAMESPACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>

typedef enum conty_ns_t {
    CONTY_NS_USER = 0,
    CONTY_NS_PID  = 1,
    CONTY_NS_MNT  = 2,
    CONTY_NS_NET  = 3,
    CONTY_NS_UTS  = 4,
    CONTY_NS_IPC  = 5,
} conty_ns_t ;

#define CONTY_NS_SIZE (CONTY_NS_IPC + 1)

#define CONTY_NS_STRLEN (sizeof("cgroup"))

static const char *conty_ns_names[CONTY_NS_STRLEN] = {
        [CONTY_NS_USER] = "user",
        [CONTY_NS_PID]  = "pid",
        [CONTY_NS_MNT]  = "mnt",
        [CONTY_NS_NET]  = "net",
        [CONTY_NS_UTS]  = "uts",
        [CONTY_NS_IPC]  = "ipc",
};

static inline int conty_ns_from_str(const char *str)
{
    for (conty_ns_t ns = 0; ns < CONTY_NS_SIZE; ns++) {
        if (strncmp(str, conty_ns_names[ns], CONTY_NS_STRLEN) == 0)
            return ns;
    }
    return -1;
}

static const int conty_ns_flags[CONTY_NS_SIZE] = {
        [CONTY_NS_USER] = CLONE_NEWUSER,
        [CONTY_NS_PID]  = CLONE_NEWPID,
        [CONTY_NS_MNT]  = CLONE_NEWNS,
        [CONTY_NS_NET]  = CLONE_NEWNET,
        [CONTY_NS_UTS]  = CLONE_NEWUTS,
        [CONTY_NS_IPC]  = CLONE_NEWIPC,
};

int conty_ns_open(conty_ns_t ns, pid_t pid);

static inline int conty_ns_open_current(conty_ns_t ns)
{
    return conty_ns_open(ns, getpid());
}

int conty_ns_unshare(int flags);
int conty_ns_set(int fd, int type);
int conty_ns_parent(int fd);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_NAMESPACE_H
