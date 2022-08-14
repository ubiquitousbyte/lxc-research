#include "sync.h"

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int test_sync()
{
    int err, status;
    int fds[2];
    pid_t child;

    if (conty_sync_init(fds) != 0)
        return -1;

    child = fork();
    if (child < 0)
        return -1;

    if (child == 0) {
        conty_sync_init_container(fds);

        err = conty_sync_wake_runtime(fds, EVENT_RT_CREATE);
        if (err != 0)
            _exit(-1);

        err = conty_sync_await_runtime(fds, EVENT_CONT_CREATE);
        if (err != 0)
            _exit(-1);

        err = conty_sync_wake_runtime(fds, EVENT_CONT_CREATED);
        if (err != 0)
            _exit(-1);

        err = conty_sync_await_runtime(fds, EVENT_CONT_START);
        if (err != 0)
            _exit(-1);

        err = conty_sync_wake_runtime(fds, EVENT_CONT_STARTED);
        if (err != 0)
            _exit(-1);

        _exit(0);
    }

    conty_sync_init_runtime(fds);

    err = conty_sync_await_container(fds, EVENT_RT_CREATE);
    if (err != 0)
        return -1;

    err = conty_sync_container(fds, EVENT_CONT_CREATE);
    if (err != 0)
        return -1;

    err = conty_sync_container(fds, EVENT_CONT_START);
    if (err != 0)
        return -1;

    if (waitpid(child, &status, 0) != child && WEXITSTATUS(status) != 0)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    if (test_sync() != 0) {
        LOG_TRACE("test_sync failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}