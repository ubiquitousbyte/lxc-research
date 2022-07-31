#include "clone.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "syscall.h"

pid_t conty_clone3(unsigned long flags, int *pidfd, void *stack, size_t stack_size)
{
    struct clone_args args = {
            .flags = flags,
            .pidfd = ptr_to_u64(pidfd),
    };

    /*
     * In clone, we always need to specify the exit signal the parent
     * should intercept when calling waitpid.
     * But if the child will be reaped by the caller's parent, instead of
     * the caller, then it will inherit its parent's exit signal
     * so clone3 fails if we try and set it. Hence, we need to check
     * and only then set it
     * See https://elixir.bootlin.com/linux/latest/source/kernel/fork.c#L2895
     */
    if (!(flags & CLONE_PARENT))
        args.exit_signal = SIGCHLD;

    if (stack) {
        args.stack = ptr_to_u64(stack);
        args.stack_size = stack_size;
    }

    /*
     * We use clone3 exclusively, We could technically fallback to the
     * legacy clone call, but that's extremely architecture-specific
     * and I simply don't have the know-how nor the energy to deal with that
     */
    return conty_raw_clone3(&args, CLONE_ARGS_SIZE_VER2);
}

int conty_clone3_cb(int (*fn)(void*), void *arg, unsigned long flags, int *pidfd,
                    void* stack, size_t stack_size)
{
    pid_t child = conty_clone3(flags, pidfd, stack, stack_size);
    if (child == 0)
        _exit(fn(arg));
    return child;
}