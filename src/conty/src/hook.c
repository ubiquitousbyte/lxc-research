#include <conty/conty.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>

#include "syscall.h"

#define CONTY_HOOK_TIMEOUT 4000

#define CONTY_HOOK_EXECVE_ARGS(hook_args, args, len) do { \
    __typeof(len) ___len = len;                           \
    args[___len] = NULL;                                  \
    struct conty_hook_param *item, *tmp;                  \
    SLIST_FOREACH_SAFE(item, hook_args, next, tmp)        \
        argv[--___len] = item->param;                     \
} while(0)

int conty_hook_init(struct conty_hook *hook, const char *path)
{
    hook->path = path;
    SLIST_INIT(&hook->args);
    SLIST_INIT(&hook->envp);
    hook->args_count = 0;
    hook->env_count = 0;
    hook->timeout_ms = CONTY_HOOK_TIMEOUT;
    return 0;
}

void conty_hook_put_arg(struct conty_hook *hook, struct conty_hook_param *arg)
{
    SLIST_INSERT_HEAD(&hook->args, arg, next);
    ++hook->args_count;
}

void conty_hook_put_env(struct conty_hook *hook, struct conty_hook_param *env)
{
    SLIST_INSERT_HEAD(&hook->envp, env, next);
    ++hook->env_count;
}

int conty_hook_put_timeout(struct conty_hook *hook, int timeout)
{
    if (timeout < 0)
        return -EINVAL;
    hook->timeout_ms = timeout;
    return 0;
}

int conty_hook_exec(struct conty_hook *hook, const char *buf, size_t buf_len)
{
    int err;
    int pipes[2];
    if (pipe(pipes) != 0)
        return -errno;

    int reader = pipes[0], writer = pipes[1];

    pid_t child = fork();
    if (child < 0)
        goto pipe_err;

    if (child == 0) {
        close(writer);
        if (dup2(reader, STDIN_FILENO) != STDIN_FILENO)
            exit(EXIT_FAILURE);

        const char *argv[hook->args_count + 2];
        argv[0] = hook->path;
        CONTY_HOOK_EXECVE_ARGS(&hook->args, argv, hook->args_count + 1);

        const char *envp[hook->env_count + 1];
        CONTY_HOOK_EXECVE_ARGS(&hook->envp, envp, hook->env_count);

        execve(hook->path, (char *const *) argv, (char *const *) envp);
        exit(EXIT_FAILURE);
    }
    close(reader);

    ssize_t tx;
    do {
        tx = write(writer, buf, buf_len);
    } while (errno == EINTR);
    err = -errno;
    close(writer);
    if (tx == -1)
        return err;

    int pid_fd = conty_pidfd_open(child, 0);
    if (pid_fd < 0)
        return -errno;

    struct pollfd pfd = {
            .fd = pid_fd,
            .events = POLLIN,
            .revents = 0
    };
    err = poll(&pfd, 1, hook->timeout_ms);
    if (err == -1)
        goto poll_err_errno;
    if (err == 0) {
        err = -ETIMEDOUT;
        goto poll_err;
    }

    if (waitpid(child, NULL, 0) == -1)
        goto poll_err_errno;

    close(pid_fd);
    return 0;

pipe_err:
    err = -errno;
    close(reader);
    close(writer);
    return err;
poll_err_errno:
    err = -errno;
poll_err:
    close(pid_fd);
    return err;
}

int conty_event_hooks_init(struct conty_event_hooks *hooks)
{
    SLIST_INIT(&hooks->on_rt_create);
    SLIST_INIT(&hooks->on_cont_created);
    SLIST_INIT(&hooks->on_cont_start);
    SLIST_INIT(&hooks->on_cont_started);
    SLIST_INIT(&hooks->on_cont_stopped);
    return 0;
}