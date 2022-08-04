#include "clone.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/signal.h>
#include <sched.h>

#include "resource.h"
#include "syscall.h"

#include <linux/sched.h>

#define __CONTY_STACK_SIZE (8 * 1024 * 1024)
pid_t conty_clone(int (*fn)(void *), void *arg, int flags, int *pidfd)
{
    __CONTY_FREE void *stack = NULL;

    stack = malloc(__CONTY_STACK_SIZE);
    if (!stack)
        return -ENOMEM;

    return clone(fn, stack + __CONTY_STACK_SIZE, flags | SIGCHLD, arg, pidfd);
}

pid_t conty_clone3(unsigned long flags, int *pidfd)
{
    struct clone_args args = {
            .flags = flags,
            .pidfd = (__u64)(uintptr_t)pidfd
    };

    if (!(flags & CLONE_PARENT))
        args.exit_signal = SIGCHLD;

    return conty_clone3_raw(&args, CLONE_ARGS_SIZE_VER2);
}

pid_t conty_clone3_cb(int (*fn)(void*), void *arg,
                      unsigned long flags, int *pidfd)
{
    pid_t child;

    child = conty_clone3(flags, pidfd);
    if (child == 0)
        _exit(fn(arg));

    return child;
}