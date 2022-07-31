#include "sandbox.h"

#include <errno.h>
#include <sys/socket.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include "clone.h"

#define CONTY_SANDBOX_IPC_SFD 0
#define CONTY_SANDBOX_IPC_RFD 1

/*
 * This is the entry point of the sandbox.
 * It resides in a namespaced environment configured by the runtime
 */
static int conty_sandbox_run(void *sb)
{
    struct conty_sandbox *sandbox = (struct conty_sandbox *) sb;

    int ipc_fd = sandbox->ipc_fds[CONTY_SANDBOX_IPC_SFD];
    close(sandbox->ipc_fds[CONTY_SANDBOX_IPC_RFD]);

    if (sandbox->ns.all_clone_flags & CLONE_NEWUSER) {
        /*
         * First order of business is to wait for the runtime to
         * configure the user and group identifiers of the user namespace.
         * This will render the sandbox privileged in its
         * environment.
         */
    }

    return 0;
}

static int __conty_sandbox_setup(void *sb)
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

        sandbox->ns.all_clone_flags |= conty_ns_type(cur);
    }

    /*
     * We want the parent of the sandbox to be the runtime, not this
     * intermediate process, so we set the CLONE_PARENT flag which makes sure
     * that the sandbox will be attached in juxtaposition to the intermediate
     * process in the process hierarchy
     */
    unsigned long flags = CLONE_PARENT | CLONE_PIDFD | sandbox->ns.new_clone_flags;
    sandbox->pid = conty_clone3_cb(conty_sandbox_run, sb, flags,
                                   &sandbox->pidfd, NULL, 0);
    if (sandbox->pid < 0)
        return -1;

    sandbox->ns.all_clone_flags |= sandbox->ns.new_clone_flags;
    return (sandbox->pid < 0) ? -1 : 0;
}

static int conty_sandbox_setup(struct conty_sandbox *sandbox)
{
    /*
     * Create a new hybrid process to set up the namespace environment and
     * spawn the actual sandbox.
     *
     * We suspend the execution of the runtime process and "borrow" its
     * memory and thread of control until the hybrid finishes
     * creating the namespaces and exits. This is enabled by CLONE_VFORK.
     *
     * For security reasons, CLONE_VFORK requires that the hybrid process
     * run in a newly-allocated stack. Otherwise, the child may corrupt
     * the runtime stack! That's why we mmap a stack.
     */
    size_t stack_len = sysconf(_SC_PAGESIZE)*2;
    int nspid_status;
    pid_t nspid;
    void *stack = mmap(NULL, stack_len, PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, 0, 0);
    if (stack == MAP_FAILED)
        return -errno;
    nspid = conty_clone3_cb(__conty_sandbox_setup, (void *) sandbox,
                            CLONE_VFORK | CLONE_VM | CLONE_FILES, NULL,
                            stack, stack_len);
    if (nspid < 0)
        goto err_out;

    if (waitpid(nspid, &nspid_status, 0) != nspid)
        goto err_out;

    if (!WIFEXITED(nspid_status) || WEXITSTATUS(nspid_status) != 0) {
        /*
         * Child couldn't join existing namespaces or start sandbox
         */
        goto err_out;
    }

    munmap(stack, stack_len);
    return 0;

err_out:
    munmap(stack, stack_len);
    return -ECHILD;
}

int conty_sandbox_create(struct conty_sandbox *sandbox)
{
    if (sandbox->ns.new_clone_flags & (CLONE_VM | CLONE_PARENT_SETTID |
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
    close(sandbox->ipc_fds[CONTY_SANDBOX_IPC_SFD]);

    int err;

    /*
     * Set up the sandbox
     */
    if ((err = conty_sandbox_setup(sandbox)) != 0)
        return err;

    /*
     * Ok, so starting from here, the sandbox should be created
     */
    return 0;
}