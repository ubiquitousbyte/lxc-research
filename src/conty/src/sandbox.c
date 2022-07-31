#include "sandbox.h"

#include <errno.h>
#include <sys/socket.h>
#include <sched.h>

#include "clone.h"

/*
 * This is the entry point of the sandbox.
 * It resides in an environment with encapsulated resources that have
 * been set up by the runtime
 */
static int conty_sandbox_run(void *sb)
{
    return 0;
}

static int conty_sandbox_join_and_run(void *sb)
{
    int err;
    struct conty_sandbox *sandbox = (struct conty_sandbox *) sb;
    struct conty_ns *cur;

    for (int i = 0; i < CONTY_SANDBOX_NS_MAX; i++) {
        cur = sandbox->ns.namespaces[i];
        if (!cur)
            continue;

        err = conty_ns_join(cur);
        if (err < 0)
            return -1;
    }
    /*
     * This is an intermediate process that simply sets up the
     * namespace environment. We want the parent of the sandbox to be
     * the runtime, not this intermediate process, so we set CLONE_PARENT
     * which makes sure that the sandbox will be attached as a sister
     * to the intermediate process in the process hierarchy
     */
    unsigned int flags = CLONE_PARENT | CLONE_PIDFD | sandbox->ns.clone_flags;
    sandbox->pid = conty_clone_cb(conty_sandbox_run, sb, flags, &sandbox->pidfd);
    return (sandbox->pid < 0) ? -1 : 0;
}

int conty_sandbox_create(struct conty_sandbox *sandbox)
{
    if (sandbox->ns.clone_flags & (CLONE_VM | CLONE_PARENT_SETTID |
                                   CLONE_CHILD_SETTID | CLONE_CHILD_CLEARTID |
                                   CLONE_SETTLS)) {
        /*
         * Caller must not be allowed to interfere with the runtime, which
         * most definitely will happen if they're allowed to access
         * its virtual memory pages or set up thread local storage
         */
        return -EINVAL;
    }

    /*
     * Create inter-process communication channel between sandbox and runtime
     */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sandbox->ipc_fds) != 0)
        return -errno;

    pid_t ns_pid;
    /*
     * Create a new hybrid process to set up the namespace environment and
     * spawn the actual sandbox. To improve efficiency, the hybrid is allowed
     * to share the parent's virtual memory pages and file descriptor table
     */
    ns_pid = conty_clone_cb(conty_sandbox_join_and_run, (void *) sandbox,
                           CLONE_VM | CLONE_VFORK | CLONE_FILES, NULL);
    if (ns_pid < 0)
        return -EINVAL;



    return 0;
}