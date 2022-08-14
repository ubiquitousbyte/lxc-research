#include "container.h"

#include <sys/stat.h>
#include <signal.h>

#include "log.h"

int test_container_lifecycle(const char *id, const char *path)
{
    int err;
    struct conty_container *cc = conty_container_create(id, path);
    if (!cc)
        return -1;

    err = conty_container_start(cc);
    if (err != 0)
        return -1;

    err = conty_container_kill(cc, SIGKILL);
    if (err != 0)
        return -1;

    err = conty_container_delete(cc);
    if (err != 0)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        LOG_ERROR("missing container config path");
        return EXIT_FAILURE;
    }

    struct stat buf;
    if (stat(argv[2], &buf) != 0) {
        LOG_ERROR("invalid container config path");
        return EXIT_FAILURE;
    }

    if (!S_ISREG(buf.st_mode)) {
        LOG_ERROR("container config path must be a file path");
        return EXIT_FAILURE;
    }

    if (test_container_lifecycle(argv[1], argv[2]) != 0)
        return -1;

    return 0;
}