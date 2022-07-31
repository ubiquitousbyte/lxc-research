#include "hook.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>

#include "syscall.h"
#include "clone.h"

#define CONTY_HOOK_TIMEOUT 4000

#define CONTY_HOOK_EXECVE_ARGS(hook_args, args, len) do { \
    __typeof(len) ___len = len;                           \
    args[___len] = NULL;                                  \
    struct conty_hook_param *item, *tmp;                  \
    SLIST_FOREACH_SAFE(item, hook_args, next, tmp)        \
        args[--___len] = item->param;                     \
} while(0)

int conty_hook_init(struct conty_hook *hook, const char *path)
{
    hook->path = path;
    SLIST_INIT(&hook->args);
    SLIST_INIT(&hook->envp);
    hook->argc = 0;
    hook->envc = 0;
    hook->timeout_ms = CONTY_HOOK_TIMEOUT;
    return 0;
}

void conty_hook_put_arg(struct conty_hook *hook, struct conty_hook_param *arg)
{
    SLIST_INSERT_HEAD(&hook->args, arg, next);
    ++hook->argc;
}

void conty_hook_put_env(struct conty_hook *hook, struct conty_hook_param *env)
{
    SLIST_INSERT_HEAD(&hook->envp, env, next);
    ++hook->envc;
}

int conty_hook_put_timeout(struct conty_hook *hook, int timeout)
{
    if (timeout < 0)
        return -EINVAL;
    hook->timeout_ms = timeout;
    return 0;
}

int conty_hook_exec(struct conty_hook *hook, const char *buf,
                    size_t buf_len, int *status)
{

    int err, pid_fd = -EBADF;
    int pipes[2];
    if (pipe(pipes) != 0)
        return -errno;

    int reader = pipes[0], writer = pipes[1];

    pid_t child = conty_clone(CLONE_PIDFD, &pid_fd);
    if (child < 0) {
        err = -errno;
        close(reader);
        close(writer);
        return err;
    }

    if (child == 0) {
        /*
         * The child will execute the hook.
         * Before that, we redirect its standard input stream to the
         * read end of the pipe where the parent will be dumping data
         */
        close(writer);
        if (dup2(reader, STDIN_FILENO) != STDIN_FILENO)
            exit(EXIT_FAILURE);

        /* 1 for the path, 1 for NULL plus everything added by put_arg */
        const char *argv[hook->argc + 2];
        argv[0] = hook->path;
        CONTY_HOOK_EXECVE_ARGS(&hook->args, argv, hook->argc + 1);

        /* 1 for NULL plus everything added by put_env */
        const char *envp[hook->envc + 1];
        CONTY_HOOK_EXECVE_ARGS(&hook->envp, envp, hook->envc);

        execve(hook->path, (char *const *) argv, (char *const *) envp);
        exit(EXIT_FAILURE);
    }

    /*
     * Parent closes the reader as it won't be reading any data and writes
     * the user-defined buffer into the pipe
     */
    close(reader);
    ssize_t tx = write(writer, buf, buf_len);
    err = -errno;
    close(writer);
    if (tx == -1)
        goto pidfd_out;

    struct pollfd pfd = {
            .fd = pid_fd,
            .events = POLLIN,
            .revents = 0
    };

    int timeout = hook->timeout_ms;
    int sig = SIGTERM;
    int poll_res;
    for (;;) {
        poll_res = poll(&pfd, 1, timeout);
        if (poll_res == -1)
            goto pidfd_err;

        if (poll_res == 0) {
            /**
             * Child hasn't exited in a timely fashion, so we'll
             * try to terminate it gracefully first and if that
             * fails we'll nail it to the ground via a SIGKILL
             */
            if (conty_pidfd_send_signal(pid_fd, sig, NULL, 0) != 0)
                goto pidfd_err;

            timeout = 0;
            sig = SIGKILL;
        }

        if (poll_res > 0) {
            /*
             * Child has terminated,
             * so we release the process identifier from the kernel
             */
            if (waitpid(child, status, 0) != child)
                goto pidfd_err;

            if (sig == SIGKILL)
                err = -ETIMEDOUT;

            goto pidfd_out;
        }
    }

pidfd_err:
    err = -errno;
pidfd_out:
    close(pid_fd);
    return err;
}

int conty_event_hooks_init(struct conty_event_hooks *hooks)
{
    SLIST_INIT(&hooks->on_rt_create);
    SLIST_INIT(&hooks->on_sandbox_created);
    SLIST_INIT(&hooks->on_sandbox_start);
    SLIST_INIT(&hooks->on_sandbox_started);
    SLIST_INIT(&hooks->on_sandbox_stopped);
    return 0;
}