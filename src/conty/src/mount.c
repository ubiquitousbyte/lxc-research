#include "mount.h"

#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <linux/mount.h>

#include "resource.h"
#include "syscall.h"
#include "log.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int __conty_bind_mount(int dfd_src, int dfd_dst,
                              unsigned long attr_set, unsigned long attr_clr,
                              unsigned long propagation, int recursive)
{
    int err;
    __CONTY_CLOSE int fd_tree_from = -EBADF;
    struct mount_attr attr = {
            .attr_set    = attr_set,
            .attr_clr    = attr_clr,
            .propagation = propagation
    };

    /*
     * Create a new mount context that is an effect identical copy
     * of the mount point defined by dfd_src.
     * The mount context is stored in kernel memory and has no
     * representation on the file system.
     */
    unsigned int ot_flags = AT_EMPTY_PATH | OPEN_TREE_CLONE | OPEN_TREE_CLOEXEC;
    fd_tree_from = conty_open_tree(dfd_src, "", ot_flags);
    if (fd_tree_from < 0)
        return LOG_ERROR_RET(-errno, "mount: could not detach fd %d", dfd_src);

    if (attr.attr_set) {
        /*
         * Configure the properties of the new mount context.
         */
        err = conty_mount_setattr(fd_tree_from, "",
                                  AT_EMPTY_PATH | (recursive ? AT_RECURSIVE : 0),
                                  &attr, sizeof(attr));
        if (err < 0)
            return LOG_ERROR_RET(-errno, "mount: could not setattr on %d", fd_tree_from);
    }

    /*
     * Attach the mount context onto the filesystem at the directory
     * specified by dfd_dst
     */
    err = conty_move_mount(fd_tree_from, "", dfd_dst, "",
                           MOVE_MOUNT_F_EMPTY_PATH | MOVE_MOUNT_T_EMPTY_PATH);

    if (err < 0)
        return LOG_ERROR_RET(-errno, "mount: %d to %d failed", fd_tree_from, dfd_dst);

    return (err < 0) ? -errno : 0;
}

int conty_bind_mount_do(const struct conty_bind_mount *mnt)
{
    __CONTY_CLOSE int fd_src = -EBADF, fd_target = -EBADF;

    fd_target = openat(-EBADF, mnt->target,
                       O_NOFOLLOW | O_PATH | O_CLOEXEC | O_DIRECTORY);
    if (fd_target < 0)
        return LOG_ERROR_RET(-errno, "mount: cannot open target %s", mnt->target);

    fd_src = openat(-EBADF, mnt->source,
                    O_NOFOLLOW | O_PATH | O_CLOEXEC | O_DIRECTORY);
    if (fd_src < 0)
        return LOG_ERROR_RET(-errno, "mount: cannot open source %s", mnt->source);

    return __conty_bind_mount(fd_src, fd_target,
                              mnt->attr_set, mnt->attr_clear,
                              mnt->propagation, 1);
}

int conty_bind_mount_undo(const struct conty_bind_mount *mnt)
{
    return umount2(mnt->target, MNT_DETACH);
}

int conty_rootfs_mount_pseudofs(const struct conty_rootfs *rootfs)
{
    int err;
    char buf[PATH_MAX];

    snprintf(buf, sizeof(buf), "%s/proc", rootfs->mnt->target);

    /*
     * Unmount the pseudo filesystem from the new root filesystem
     * if it was somehow mounted before
     */
    err = umount2(buf, MNT_DETACH);
    if (err)
        LOG_INFO("mount: skipping umount for %s", buf);

    /*
     * Try to create pseudo filesystem directory under the configured
     * root filesystem if there isn't one
     */
    err = mkdirat(rootfs->dfd_mnt, "proc",
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (err < 0 && errno != EEXIST)
        return LOG_ERROR_RET(-errno, "mount: cannot create dentry %s", buf);

    /*
     * Mount proc onto the new root filesystem
     */
    err = mount("proc", buf, "proc", MS_NOEXEC | MS_NOSUID | MS_NODEV, NULL);
    if (err != 0)
        return LOG_ERROR_RET(-errno, "mount: cannot mount procfs on %s", buf);

    /*
     * Reset the buffer and do the same thing with sysfs
     */
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%s/sys", rootfs->mnt->target);

    umount2(buf, MNT_DETACH);

    err = mkdirat(rootfs->dfd_mnt, "sys",
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (err < 0 && errno != EEXIST)
        return LOG_ERROR_RET(-errno, "mount: cannot create dentry %s", buf);

    err = mount("sysfs", buf, "sysfs", 0, NULL);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "mount: cannot mount sysfs on %s", buf);

    return 0;
}

int conty_rootfs_pivot(const struct conty_rootfs *rootfs)
{
    __CONTY_CLOSE int old_root = -EBADF;
    int err;

    /*
     * Open the old root mount which we'll unmount at the end
     * We need to keep the file descriptor alive, otherwise
     * after pivot_root the old mount files won't be accessible anymore
     */
    old_root = openat(-EBADF, "/", O_NOFOLLOW | O_PATH | O_CLOEXEC | O_DIRECTORY);
    if (old_root < 0)
        return LOG_ERROR_RET(-errno, "pivot: cannot open old rootfs");

    /*
     * Switch to the rootfs directory
     */
    err = fchdir(rootfs->dfd_mnt);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "pivot: cannot chdir to %s", rootfs->mnt->target);

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
        return LOG_ERROR_RET(-errno, "pivot: cannot chroot");

    /*
     * Now change dir to old root, which is technically layered on top of
     * the new root. We can't access it through a path, because the kernel
     * will always resolve "/"  to the new root, so we
     * chdir via the file descriptor we opened before pivoting
     */
    err = fchdir(old_root);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "pivot: cannot umount old root");

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
        return LOG_ERROR_RET(-errno, "pivot: cannot umount old root");

    /*
     * Now it's safe to unmount old_root
     */
    err = umount2(".", MNT_DETACH);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "pivot: cannot umount old root");

    err = fchdir(rootfs->dfd_mnt);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "pivot: cannot switch to new root");

    return err;
}