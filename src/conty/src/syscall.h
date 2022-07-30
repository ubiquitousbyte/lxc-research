#ifndef CONTY_SYSCALL_H
#define CONTY_SYSCALL_H

#ifdef __cplusplus
extern "C" {
#endif

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

static inline int conty_fsopen(const char *fsname, unsigned int flags)
{
    return syscall(SYS_fsopen, fsname, flags);
}

static inline int conty_fspick(int dfd, const char *path, unsigned int flags)
{
    return syscall(SYS_fspick, dfd, path, flags);
}

static inline int conty_fsconfig(int fd, unsigned int cmd,
                                 const char *key, const char *value, int aux)
{
    return syscall(SYS_fsconfig, fd, cmd, key, value, aux);
}

static inline int conty_fsmount(int fs_fd, unsigned int flags, unsigned int ms_flags)
{
    return syscall(SYS_fsmount, fs_fd, flags, ms_flags);
}

static inline int conty_open_tree(int dfd, const char *path, unsigned flags)
{
    return syscall(SYS_open_tree, dfd, path, flags);
}

static inline int conty_move_mount(int from_dfd, const char *from_pathname,
                                   int to_dfd, const char *to_pathname,
                                   unsigned int flags)
{
    return syscall(SYS_move_mount, from_dfd, from_pathname,
                   to_dfd, to_pathname, flags);
}

static inline int conty_pivot_root(const char *new_root, const char *put_old)
{
    return syscall(SYS_pivot_root, new_root, put_old);
}

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_SYSCALL_H
