#include "clone.h"
#include "resource.h"
#include "log.h"

#include <sys/wait.h>
#include <sched.h>

int child_func(void *arg)
{
    return 0;
}

static int test_clone_old()
{
    pid_t child;
    FD_RESOURCE int pollfd = -EBADF;

    child = clone_old(child_func, NULL,
                      CLONE_NEWUSER | CLONE_NEWUTS | CLONE_PIDFD, &pollfd);
    if (child < 0)
        return -1;

    if (pollfd < 0)
        return -1;

    if (waitpid(child, NULL, 0) != child)
        return -1;

    return 0;
}

static int test_clone3_cb()
{
    pid_t child;
    FD_RESOURCE int pollfd = -EBADF;

    child = clone3_cb(child_func, NULL,
                      CLONE_NEWUSER | CLONE_NEWUTS | CLONE_PIDFD, &pollfd);
    if (child < 0)
        return -1;

    if (pollfd < 0)
        return -1;

    if (waitpid(child, NULL, 0) != child)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    if (test_clone_old() != 0) {
        LOG_ERROR("test_clone_old failed");
        goto err;
    }

    if (test_clone3_cb() != 0) {
        LOG_ERROR("test_clone_cb failed");
        goto err;
    }

    return EXIT_SUCCESS;

err:
    return EXIT_FAILURE;
}