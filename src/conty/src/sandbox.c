#include "sandbox.h"

#include <errno.h>
#include <sys/socket.h>
#include <sched.h>
#include <stdlib.h>

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
            return EXIT_FAILURE;
    }

    /*
     * This is an intermediate process that simply sets up the
     * namespace environment. We want the parent of the sandbox to be
     * the runtime, not this intermediate process, so we set CLONE_PARENT
     * which makes sure that the sandbox will be attached as a sister
     * to the intermediate process, rather than a child.
     */
    unsigned int flags = CLONE_PARENT | sandbox->ns.clone_flags;
    err = conty_clone_cb(conty_sandbox_run, sb, flags, &sandbox->pidfd);
    
    return (err < 0) ? EXIT_FAILURE : 0;
}

int conty_sandbox_create(struct conty_sandbox *sandbox)
{
    int err;
    err = socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sandbox->ipc_fds);
    if (err != 0)
        return -errno;

    return 0;

}