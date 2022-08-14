#include "oci.h"

#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/poll.h>

#include "resource.h"
#include "log.h"
#include "clone.h"

int oci_hook_exec(struct oci_hook *hook, const struct oci_process_state *state)
{
    MEM_RESOURCE char *buf = NULL;
    FD_RESOURCE int hkfd, reader, writer = -EBADF;
    int ipc[2];
    int sig = SIGTERM, err, status;
    pid_t hkpid;
    size_t buflen;
    struct pollfd pollfd;
    unsigned int timeout = hook->ohk_timeout;

    if (!(buf = oci_process_state_ser(state, &buflen)))
        return -EINVAL;

    /*
     * Construct a pipe that we'll use to redirect the standard input stream
     * of the hook and write the process state into it
     */
    if (pipe(ipc) != 0)
        return log_error_ret(-errno, "cannot execute hook %s", hook->ohk_path);

    reader = ipc[0], writer = ipc[1];

    hkpid = clone3_ret(CLONE_PIDFD, &hkfd);
    if (hkpid < 0)
        return log_error_ret(-errno, "cannot execute hook %s", hook->ohk_path);

    if (hkpid == 0) {
        close(writer);
        /* Adjust stdin stream of hook */
        dup2(reader, STDIN_FILENO);
        close(reader);
        /* Execute the hook */
        execve(hook->ohk_path, hook->ohk_argv, hook->ohk_envp);
        /* We should never reach this point */
        _exit(-1);
    }

    close(move_fd(reader));
    /*
     * Write the process state into the hook
     */
    ssize_t tx = write(writer, buf, buflen);
    fd_cleaner_func(&writer);
    if (tx < 0) {
        err = -errno;
        goto err_out;
    }

    pollfd = (struct pollfd) { .fd = hkfd, .events = POLLIN, .revents = 0};
    timeout *= 1000;
    for (;;) {
        err = poll(&pollfd, 1, (int) timeout);
        if (err == -1) {
            err = -errno;
            goto err_out;
        }

        if (err == 0) {
            /*
             * The hook timed out, try to terminate it gracefully
             * and nail it to the ground if that fails
             */
            if (pidfd_send_signal(hkfd, sig, NULL, 0) != 0)
                return log_error_ret(-errno, "cannot terminate hook");
            timeout = 0;
            sig = SIGKILL;
        }

        if (err > 0) {
            if (waitpid(hkpid, &status, 0) != hkpid)
                return -errno;

            return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : -1;
        }
    }

err_out:
    if (pidfd_send_signal(hkfd, SIGKILL, NULL, 0) != 0)
        LOG_WARN("cannot kill hook process after error in parent");

    if (waitpid(hkpid, NULL, 0) != hkpid)
        LOG_WARN("cannot await hook process after error in parent");

    return err;
}