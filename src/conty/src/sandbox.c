#include "sandbox.h"

#include <sched.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "clone.h"
#include "user.h"
#include "syscall.h"
#include "mount.h"

static int conty_sandbox_join_existing(const struct conty_sandbox *sandbox)
{
    for (conty_ns_t ns = 0; ns < CONTY_NS_SIZE; ns++) {
        if (sandbox->ns_fds[ns] >= 0)
            return 1;
    }
    return 0;
}

static int conty_sandbox_write_mappings(const struct conty_sandbox *sandbox)
{
    int err = 0;
    if (sandbox->id_map.users) {
        err = conty_user_write_uid_mappings(sandbox->pid, sandbox->id_map.users);
        if (err != 0)
            return -1;
    }

    if (sandbox->id_map.groups) {
        err = conty_user_disable_setgroups(sandbox->pid);
        if (err != 0)
            return -1;

        err = conty_user_write_gid_mappings(sandbox->pid,
                                            sandbox->id_map.groups);
    }

    return err;
}

static int conty_sandbox_mount_rootfs(struct conty_sandbox *sandbox)
{
    int err;
    struct conty_rootfs *rootfs = &sandbox->rootfs;

    if ((err = conty_bind_mount_do(sandbox->rootfs.mnt)) != 0)
        return err;

    sandbox->rootfs.dfd_mnt = openat(-EBADF, sandbox->rootfs.mnt->target,
                                     O_NOFOLLOW | O_PATH | O_CLOEXEC | O_DIRECTORY);

    return (sandbox->rootfs.dfd_mnt < 0) ? -errno : 0;
}

static int conty_sandbox_configure(struct conty_sandbox *sandbox)
{
    /*
     * Use the raw getpid system call, because glibc might cache pids and
     * we don't want the sandbox to think that it's the runtime..
     */
    sandbox->pid = conty_getpid();

    if (sandbox->ns_new & CLONE_NEWUSER) {
        /*
         * Write identifier mappings between parent and sandbox
         */
        if (conty_sandbox_write_mappings(sandbox) != 0)
            goto err_notify_rt;
    }

    if (sandbox->ns_new & CLONE_NEWUTS) {
        /*
         * Configure sandbox hostname
         */
        if (sethostname(sandbox->hostname, strlen(sandbox->hostname)) != 0)
            goto err_notify_rt;
    }

    if (sandbox->ns_new & CLONE_NEWNS) {
        /*
         * Bind mount the root filesystem into the sandbox mount namespace
         */
        if (conty_sandbox_mount_rootfs(sandbox) != 0)
            goto err_notify_rt;

        /*
         * Mount all pseudo filesystems like proc and sys
         */
        if (conty_rootfs_mount_pseudofs(&sandbox->rootfs) != 0)
            goto err_notify_rt;

        /*
         * Pivot root into new root filesystem
         */
        if (conty_rootfs_pivot(&sandbox->rootfs) != 0)
            goto err_notify_rt;
    }

err_notify_rt:
    return -1;
}

/*
 * The entry point of the sandbox
 * The execution context resides in a namespaced environment configured by
 * the runtime.
 */
int __conty_sandbox_run(void *sb)
{
    struct conty_sandbox *sandbox = (struct conty_sandbox *) sb;
    sandbox->pid = conty_getpid();

    if (sandbox->ns_new & CLONE_NEWUSER) {
        /*
         * Configure identifier mappings between the parent
         * user namespace and that of the sandbox.
         */
        if (conty_sandbox_write_mappings(sandbox) != 0)
            return -1;

    }

    if (sandbox->ns_new & CLONE_NEWUTS) {
        /*
         * Configure ns_new hostname
         */
        if (sethostname(sandbox->hostname, strlen(sandbox->hostname)) != 0)
            return -1;
    }

    return 0;
}

int __conty_sandbox_setup(void *sb)
{
    int nsfd;
    struct conty_sandbox *sandbox = (struct conty_sandbox *) sb;

    sandbox->ns_old = 0;
    for (conty_ns_t ns = 0; ns < CONTY_NS_SIZE; ns++) {
        nsfd = sandbox->ns_fds[ns];
        if (nsfd < 0)
            continue;

        if (conty_ns_set(nsfd, 0) < 0)
            return -1;

        sandbox->ns_old |= conty_ns_flags[ns];
    }

    /*
     * We want the parent of the sandbox to be the runtime, not this
     * intermediate process, so we set the CLONE_PARENT flag which makes
     * sure that the sandbox will be attached in juxtaposition to the
     * intermediate process in the process hierarchy
     */
    sandbox->pid = conty_clone3_cb(__conty_sandbox_run, sandbox,
                                   sandbox->ns_new | CLONE_PARENT | CLONE_PIDFD,
                                   &sandbox->pidfd);

    return (sandbox->pid < 0) ? -1: 0;
}

int conty_sandbox_start(struct conty_sandbox *sandbox)
{
    /*
     * First, we'll create a connected socket pair to be used by the
     * runtime and the sandbox for communication
     */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sandbox->ipc_fds) != 0)
        return -1;

    if (conty_sandbox_join_existing(sandbox)) {
        /*
         * The sandbox needs to join an already existing set of namespaces
         * This necessitates the creation of an intermediate process
         * that joins the existing namespaces and spawns the sandbox, which
         * will inherit the environment.
         *
         * We do a little performance trick here and suspend the execution
         * of the runtime process and "borrow" its memory and thread of
         * control until the intermediate process finishes creating the
         * namespaces and exists. That way, copy-on-write semantics
         * aren't employed and the kernel does not have to copy the
         * runtime's entire set of virtual memory pages.
         *
         * We do, however, have to setup a stack for the child. Otherwise,
         * the intermediate process will most definitely corrupt the runtime stack.
         */
        int hybrid_status;
        pid_t hybrid;

        hybrid = conty_clone(__conty_sandbox_setup, sandbox,
                    CLONE_VFORK | CLONE_VM | CLONE_FILES, NULL);

        if (hybrid < 0)
            return -1;

        if (waitpid(hybrid, &hybrid_status, 0) != hybrid)
            return -1;

        if (!WIFEXITED(hybrid_status) || WEXITSTATUS(hybrid_status) != 0) {
            /*
             * Child couldn't join existing namespaces or start sandbox
             */
            return -1;
        }
    } else {
        /*
         * The sandbox will not be joining any existing namespaces so
         * we can directly clone it here
         */
        sandbox->pid = conty_clone3_cb(__conty_sandbox_run, sandbox,
                                       sandbox->ns_new, &sandbox->pidfd);
        if (sandbox->pid < 0)
            return -1;
    }

    return 0;
}