#ifndef CONTY_SYSCALL_H
#define CONTY_SYSCALL_H

#include <sys/syscall.h>
#include <unistd.h>

static inline int conty_pidfd_open(pid_t pid, unsigned int flags)
{
    return syscall(SYS_pidfd_open, pid, flags);
}

#endif //CONTY_SYSCALL_H
