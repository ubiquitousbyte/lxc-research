#include "clone.h"

#include <errno.h>
#include <stdint.h>
#include <sched.h>
#include <sys/signal.h>

#include <linux/types.h>
#include <linux/sched.h>

#include "resource.h"

pid_t clone3_ret(unsigned long flags, int *pidfd)
{
    pid_t child;
    struct clone_args args = {
            flags = flags,
            .pidfd = (__u64)(uintptr_t)pidfd
    };

    /*
     * Fun fact, we can't set the exit signal for a task that has the
     * CLONE_PARENT flag set, because it inherits the same exit signal
     * as that of the calling process.
     *
     * The kernel considers this an error, ergo the if statement
     */
    if (!(flags & CLONE_PARENT))
        args.exit_signal = SIGCHLD;

    child = clone3(&args, CLONE_ARGS_SIZE_VER2);
    return (child < 0) ? -errno : child;
}

pid_t clone3_cb(int (*fn)(void*), void *udata, unsigned long flags, int *pidfd)
{
    pid_t child;

    if ((child = clone3_ret(flags, pidfd)) == 0)
        _exit(fn(udata));

    return child;
}

pid_t clone_old(int (*fn)(void*), void *udata, int flags, int *pidfd)
{
    /* Typical stack size on a Linux box is this plus we don't need more */
    static const size_t STACK_SIZE = 8 * 1024 * 1024;
    MEM_RESOURCE void *stack = NULL;
    pid_t child;

    stack = malloc(STACK_SIZE);
    if (!stack)
        return -ENOMEM;

    /*
     * Stack grows downwards but the wrapper expects the initial address
     * according to the virtual memory layout, which is the top most address,
     * so we pass stack + STACK_SIZE.
     *
     * Some architectures don't necessitate this, but arm64 and x86_64 do and
     * that's who we're targeting
     */
    child = clone(fn, stack + STACK_SIZE, flags | SIGCHLD, udata, pidfd);
    return (child < 0) ? -errno : child;
}