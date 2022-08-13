#ifndef CONTY_CLONE_H
#define CONTY_CLONE_H

#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>

/*
 * Forward-declaring this here because <sched.h> and <linux/sched.h>
 * don't play well together and I don't want to expose either of them
 * in a header file.
 * The compiler will always find a definition for this because
 * <linux/sched.h> is included in the C-file
 */
struct clone_args;

/*
 * The clone3 system call provides a flexible interface for creating
 * tasks inside the kernel.
 * Unlike the glibc wrapper, the raw clone3 system call does not necessitate
 * the creation of a separate stack for the new task.
 */
static inline pid_t clone3(struct clone_args *args, size_t args_size)
{
    return (pid_t) syscall(SYS_clone3, args, args_size);
}

/*
 * Creates a new task that commences execution by calling the function fn
 * with the user-defined argument udata.
 * The execution context of the task is controlled by flags and
 * the user can optionally request a file descriptor that refers to the new task
 * Under the hood, this function calls the raw clone3 system call
 */
pid_t clone3_cb(int (*fn)(void*), void *udata, unsigned long flags, int *pidfd);

pid_t clone3_ret(unsigned long flags, int *pidfd);

/*
 * See clone3_cb.
 * This acts exactly the same way, but under the hood, the glibc wrapper is used.
 *
 * NOTE:
 * We use this when we spawn a container that has to join
 * already existing namespaces. We clone an intermediate process that calls
 * setns and spawns the actual container process.
 * To improve performance, the intermediate process is cloned with CLONE_VFORK
 * and CLONE_VM set, which means that it suspends its parent and reuses its
 * virtual memory pages to avoid redundant page copying until it finishes
 * execution by calling _exit. However, this means that the intermediate process
 * needs to jump to a new stack address (otherwise it would overwrite the parent's stack..),
 * which requires a tiny bit of assembly that we don't want to write.
 * The glibc wrapper, however, does it for us:
 * https://code.woboq.org/userspace/glibc/sysdeps/unix/sysv/linux/x86_64/clone.S.html
 */
pid_t clone_old(int (*fn)(void*), void *udata, int flags, int *pidfd);

/*
 * Send a signal to a process via a pollable file descriptor
 */
static inline int pidfd_send_signal(int pid_fd, int sig, siginfo_t *info,
                                    unsigned int flags)
{
    return (int) syscall(SYS_pidfd_send_signal, pid_fd, sig, info, flags);
}
#endif //CONTY_CLONE_H
