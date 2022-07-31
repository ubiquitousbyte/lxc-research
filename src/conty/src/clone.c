#include "clone.h"

#include <errno.h>
#include <stdint.h>

#include "syscall.h"

pid_t conty_clone(unsigned long flags, int *pidfd)
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
     * so there is no need to set it in that case
     */
    if (!(flags & CLONE_PARENT))
        args.exit_signal = SIGCHLD;

    /*
     * We use clone3 exclusively, We could technically fallback to the
     * legacy clone call, but that's extremely architecture-specific
     * and I simply don't have the know-how nor the energy to deal with that
     */
    return conty_raw_clone3(&args, CLONE_ARGS_SIZE_VER0);
}

int conty_clone_cb(int (*fn)(void*), void *arg, unsigned int flags, int *pidfd)
{
    pid_t child = conty_clone(flags, pidfd);
    if (child == 0)
        _exit(fn(arg));
    return child;
}