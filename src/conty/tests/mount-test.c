#include "mount.h"

#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "clone.h"
#include "log.h"
#include "safestring.h"

static int rootfs_mounter(void *dst)
{
    const char *cro_dst = (const char *) dst;
    int err;
    struct conty_rootfs rfs;

    if ((err = conty_rootfs_init(&rfs, cro_dst, 0)) != 0)
        return err;

    if ((err = conty_rootfs_mount(&rfs)) != 0)
        return err;

    char p[PATH_MAX];
    err = snprintf(p, sizeof(p), "%s/random_file", cro_dst);
    if (err < 0)
        return -1;

    int fd = creat(p, 0755);
    if (fd < 0)
        return -1;

    close(fd);

    return 0;
}

static int test_rootfs_mount(char *cro_dst)
{
    int status;
    pid_t child;

    child = clone3_cb(rootfs_mounter, cro_dst, CLONE_NEWNS, NULL);

    if (waitpid(child, &status, 0) != child)
        return -1;

    if (WEXITSTATUS(status) != 0)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        LOG_ERROR("missing rootfs path");
        return EXIT_FAILURE;
    }

    struct stat buf;
    if (stat(argv[1], &buf) != 0) {
        LOG_ERROR("invalid rootfs path");
        return EXIT_FAILURE;
    }

    if (S_ISREG(buf.st_mode)) {
        LOG_ERROR("rootfs must be directory");
        return EXIT_FAILURE;
    }

    if (test_rootfs_mount(argv[1]) != 0) {
        LOG_ERROR("test_rootfs_mount failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
