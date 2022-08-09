#include <conty/conty.h>

#include <sys/wait.h>
#include <sys/poll.h>
#include <unistd.h>

#include "resource.h"
#include "syscall.h"

int conty_hook_exec(const char *prog, const char *argv[], const char *envp[],
                    const char *buf, size_t buf_len, int timeout)
{
    __CONTY_CLOSE int hook_fd = -EBADF;
    int ipc[2];
    int read_end, write_end, sig = SIGTERM, perr, status;
    struct pollfd poll_fd;

    if (pipe(ipc) != 0)
        return -errno;
    read_end = ipc[0], write_end = ipc[1];

    pid_t hook_pid = fork();
    if (hook_pid < 0)
        return -errno;

    if (hook_pid == 0) {
        close(write_end);
        dup2(read_end, STDIN_FILENO);
        close(read_end);
        execve(prog, (char *const *) argv, (char *const *) envp);
        _exit(-1);
    }

    close(read_end);
    ssize_t tx = write(write_end, buf, buf_len);
    conty_close_fd_function(&write_end);
    if (tx < 0)
        return -errno;

    if ((hook_fd = conty_pidfd_open(hook_pid, 0)) < 0)
        return -errno;

    poll_fd = (struct pollfd) { .fd = hook_fd, .events = POLLIN, .revents = 0 };
    timeout *= 1000;
    for (;;) {
        perr = poll(&poll_fd, 1, timeout);
        if (perr == -1)
            return -errno;

        if (perr == 0) {
            if (conty_pidfd_send_signal(hook_fd, sig, NULL, 0) != 0)
                return -errno;
            timeout = 0;
            sig = SIGKILL;
        }

        if (perr > 0) {
            if (waitpid(hook_pid, &status, 0) != hook_pid)
                return -errno;

            if (WIFSIGNALED(status)) {
                if (WTERMSIG(status) == SIGKILL || WTERMSIG(status) == SIGTERM)
                    return -ETIME;
                return -errno;
            }

            return (WIFEXITED(status) && WEXITSTATUS(status) != 0) ? -1 : 0;
        }
    }
}

int conty_oci_hook_exec(const struct oci_hook *hook,
                        const struct oci_process_state *state)
{
    size_t state_buf_len;
    unsigned int timeout;
    __CONTY_FREE char *state_buf = NULL;

    state_buf = oci_ser_process_state(state, &state_buf_len);
    if (!state_buf)
        return -EINVAL;

    timeout = (hook->oh_timeout == 0) ? CONTY_HOOK_DEFAULT_TIMEOUT : hook->oh_timeout;

    return conty_hook_exec(hook->oh_path, (const char **) hook->oh_argv,
                           (const char **) hook->oh_envp, state_buf,
                           state_buf_len, timeout);
}
