#include "mount.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <linux/mount.h>

#include "resource.h"
#include "syscall.h"
#include "log.h"

int conty_rootfs_pivot(const struct conty_rootfs *rootfs)
{
    __CONTY_CLOSE int old_root = -EBADF, target = -EBADF;
    int err;

    /*
     * We need a file descriptor reference in order to chdir back to
     * the new root filesystem once we unmount the old one.
     * If we try and reference the new root by path after pivoting that won't
     * work anymore because well.. we pivoted
     */
    target = openat(-EBADF, rootfs->crfs_target,
                    O_DIRECTORY | O_PATH | O_CLOEXEC | O_NOFOLLOW);
    if (target < 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot open rootfs");

    /*
     * Open the old root mount which we'll unmount at the end
     * We need to keep the file descriptor alive, otherwise
     * after pivot_root the old mount files won't be accessible anymore
     */
    old_root = openat(-EBADF, "/", O_DIRECTORY | O_PATH | O_CLOEXEC | O_NOFOLLOW);
    if (old_root < 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot open old rootfs");

    /*
     * Switch to the rootfs directory
     */
    err = chdir(rootfs->crfs_target);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot chdir");

    /*
     * Pivot root into the current directory, which happens to be our
     * the root directory of the new root filesystem.
     *
     * Also put the old root filesystem on top of the current one.
     * to avoid having to create a temporary directory for the old rootfs.
     * Kudos to LXC and runc for mentioning this:
     *  https://github.com/opencontainers/runc/pull/1125
     *
     * Note that the old root filesystem will remain mounted on top of the new
     * one at the same location. This causes the paths to be weirdly
     * intermingled, e.g:
     * $ cd ..
     * $ pwd
     * ## /
     * will put you in the old root from the new root,
     * but then if you do an absolute change dir:
     * $ cd /
     * you'll end up back in the new root..
     *
     * Unmounting the old root after pivoting fixes this
     */
    err = conty_pivot_root(".", ".");
    if (err < 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot pivot root");

    /*
     * Now change dir to old root, which is technically layered on top of
     * the new root. We can't access it through a path, because the kernel
     * will always resolve "/"  to the new root, so we
     * chdir via the file descriptor we opened before pivoting
     */
    err = fchdir(old_root);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot umount old root");

    /*
     * The old root filesystem
     * may have filesystem propagation events turned on which we've
     * inherited when creating the new mount namespace.
     * That means that if we unmount the old root, the umount event
     * will be propagated to the host, because it's in the same peer group.
     * Soo.. try and guess what will happen if the root filesystem
     * that the sandbox is using is a copy of the root filesystem on the host.
     * The host will react to the umount event by unmounting its
     * root filesystem, which is really bad.
     *
     * To fix this, we'll change the configuration of the old root filesystem
     * inside the new mount namespace to slave. That means
     * that events will be propagated only from the host down, and not
     * from the sandbox up
     */
    err = mount("", ".", "", MS_SLAVE | MS_REC, NULL);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot umount old root");

    /*
     * Now it's safe to unmount old_root
     */
    err = umount2(".", MNT_DETACH);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot umount old root");

    err = fchdir(target);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot switch to new root");

    return err;
}

int conty_rootfs_mount(const struct conty_rootfs *rootfs)
{
    int err;

    err = mkdir(rootfs->crfs_target, 0755);
    if (err != 0 && errno != EEXIST)
        return LOG_ERROR_RET(-errno, "cannot mkdir rootfs %s", rootfs->crfs_target);

    unsigned long mflags = MS_BIND | MS_REC | MS_PRIVATE;
    mflags |= (rootfs->crfs_readonly) ? MS_RDONLY : 0;

    err = mount(rootfs->crfs_source, rootfs->crfs_target, "bind", mflags, NULL);
    if (err != 0) {
        return LOG_ERROR_RET(-errno, "cannot bind mount %s<->%s",
                             rootfs->crfs_source, rootfs->crfs_target);
    }

    return err;
}

int conty_rootfs_mount_devfs(struct conty_rootfs *rootfs)
{
    int err;
    char buf[PATH_MAX];

    snprintf(buf, sizeof(buf), "%s/dev", rootfs->crfs_target);

    err = mkdir(buf, 0755);
    if (err != 0 && errno != EEXIST)
        return LOG_ERROR_RET(-errno, "cannot mkdir dev directory");

    err = mount("none", buf, "tmpfs", 0, "mode=0755,size=500000");
    if (err != 0)
        return LOG_ERROR_RET(-errno, "cannot mount devfs");

    return 0;
}

int conty_rootfs_mkdevices(struct conty_rootfs *rootfs)
{
    static const char *devices[] = {
            "null",
            "zero",
            "full",
            "random",
            "urandom",
            "tty"
    };

    char host_path[PATH_MAX];
    char cont_path[PATH_MAX];
    int err;

    for (int i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
        __CONTY_CLOSE int fd = -EBADF;

        snprintf(cont_path, PATH_MAX, "%s/dev/%s",
                 rootfs->crfs_target, devices[i]);

        snprintf(host_path, PATH_MAX, "/dev/%s", devices[i]);

        fd = open(cont_path, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd < 0 && errno != EEXIST)
            return LOG_ERROR_RET(-errno, "cannot open/create device %s: %s", cont_path, strerror(errno));

        err = mount(host_path, cont_path, 0, MS_BIND, NULL);
        if (err != 0)
            return LOG_ERROR_RET(-errno, "cannot bind mount %s<->%s", host_path, cont_path);
    }

    return 0;
}