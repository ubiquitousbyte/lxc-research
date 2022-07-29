#ifndef CONTY_SYSCALL_H
#define CONTY_SYSCALL_H

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

static inline int conty_pidfd_open(pid_t pid, unsigned int flags)
{
    return syscall(SYS_pidfd_open, pid, flags);
}

static inline int conty_pidfd_send_signal(int pid_fd, int sig,
                                          siginfo_t *info, unsigned int flags)
{
    return syscall(SYS_pidfd_send_signal, pid_fd, sig, info, flags);
}

static inline int fsopen(const char *fsname, unsigned int flags)
{
    return syscall(SYS_fsopen, fsname, flags);
}

static inline int fspick(int dfd, const char *path, unsigned int flags)
{
    return syscall(SYS_fspick, dfd, path, flags);
}

static inline int fsconfig(int fd, unsigned int cmd,
                           const char *key, const char *value, int aux)
{
    return syscall(SYS_fsconfig, fd, cmd, key, value, aux);
}

#endif //CONTY_SYSCALL_H
