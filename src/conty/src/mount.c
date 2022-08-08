#include "mount.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

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
        return -errno;

    if (attr.attr_set) {
        /*
         * Configure the properties of the new mount context.
         */
        err = conty_mount_setattr(fd_tree_from, "",
                                  AT_EMPTY_PATH | (recursive ? AT_RECURSIVE : 0),
                                  &attr, sizeof(attr));
        if (err < 0)
            return -errno;
    }

    /*
     * Attach the mount context onto the filesystem at the directory
     * specified by dfd_dst
     */
    err = conty_move_mount(fd_tree_from, "", dfd_dst, "",
                           MOVE_MOUNT_F_EMPTY_PATH | MOVE_MOUNT_T_EMPTY_PATH);

    return (err < 0) ? -errno : 0;
}

void conty_rootfs_init_runtime(struct conty_rootfs *rootfs, const char *source,
                              const char *target, char readonly)
{
    rootfs->cr_source = source;
    rootfs->cr_target = target;
    rootfs->cr_ro = readonly;
    rootfs->cr_dfd_mnt = -EBADF;
}

int conty_rootfs_init_container(struct conty_rootfs *rootfs)
{
    rootfs->cr_dfd_mnt = openat(-EBADF, rootfs->cr_target,
                                O_NOFOLLOW | O_PATH | O_CLOEXEC | O_DIRECTORY);
    if (rootfs->cr_dfd_mnt < 0) {
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot open %s: %s",
                             rootfs->cr_target, strerror(errno));
    }

    return 0;
}

int conty_rootfs_mount(const struct conty_rootfs *rootfs)
{
    int err;
    __CONTY_CLOSE int fd_src = -EBADF;

    fd_src = openat(-EBADF, rootfs->cr_source,
                    O_NOFOLLOW | O_PATH | O_CLOEXEC | O_DIRECTORY);
    if (fd_src < 0) {
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot open src %s: %s",
                             rootfs->cr_source, strerror(errno));
    }

    err = __conty_bind_mount(fd_src, rootfs->cr_dfd_mnt,
                              (rootfs->cr_ro) ? MS_RDONLY : 0, 0, MS_PRIVATE, 1);
    if (err != 0) {
        return LOG_ERROR_RET(err, "conty_rootfs: cannot bind mount %s<->%s: %s",
                             rootfs->cr_source, rootfs->cr_target, strerror(-err));
    }

    return err;
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
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot open old rootfs");

    /*
     * Switch to the rootfs directory
     */
    err = fchdir(rootfs->cr_dfd_mnt);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot chdir to %s", rootfs->cr_target);

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

    err = fchdir(rootfs->cr_dfd_mnt);
    if (err < 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot switch to new root");

    return err;
}

int conty_rootfs_mount_devices(const struct conty_rootfs *rootfs)
{
    static const struct {
        const char  *cd_name;
        mode_t       cd_type;
        unsigned int cd_major;
        unsigned int cd_minor;
    } devices[] = {
            { .cd_name = "null",    .cd_type = S_IFCHR, .cd_major = 1, .cd_minor = 5 },
            { .cd_name = "zero",    .cd_type = S_IFCHR, .cd_major = 1, .cd_minor = 7 },
            { .cd_name = "full",    .cd_type = S_IFCHR, .cd_major = 1, .cd_minor = 7 },
            { .cd_name = "random",  .cd_type = S_IFCHR, .cd_major = 1, .cd_minor = 8 },
            { .cd_name = "urandom", .cd_type = S_IFCHR, .cd_major = 1, .cd_minor = 9 },
            { .cd_name = "tty",     .cd_type = S_IFCHR, .cd_major = 5, .cd_minor = 0 },
    };
    int __CONTY_CLOSE dev_dir = -EBADF;
    dev_t cd_dev_type;
    mode_t cd_mode;
    int err;

    dev_dir = openat(rootfs->cr_dfd_mnt, "dev",
                     O_NOFOLLOW | O_PATH | O_CLOEXEC | O_DIRECTORY);

    if (dev_dir < 0) {
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot open %s/dev: %s",
                             rootfs->cr_target, strerror(errno));
    }

    for (int i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
        cd_dev_type = makedev(devices[i].cd_major, devices[i].cd_minor);
        cd_mode = devices[i].cd_type;

        err = mknodat(dev_dir, devices[i].cd_name, cd_mode, cd_dev_type);
        if (err != 0)
            return -errno;
    }

    return 0;
}

int conty_rootfs_mount_proc(const struct conty_rootfs *rootfs)
{
    int err;
    char buf[PATH_MAX];

    snprintf(buf, sizeof(buf), "%s/proc", rootfs->cr_target);

    err = umount2(buf, MNT_DETACH);
    if (err)
        LOG_INFO("conty_rootfs: skipping umount for %s", buf);

    err = mkdirat(rootfs->cr_dfd_mnt, "proc",
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (err < 0 && errno != EEXIST)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot create dentry %s", buf);

    err = mount("proc", buf, "proc", MS_NOEXEC | MS_NOSUID | MS_NODEV, NULL);
    if (err != 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot mount proc at %s", buf);

    return err;
}

int conty_rootfs_mount_sys(const struct conty_rootfs *rootfs)
{
    int err;
    char buf[PATH_MAX];

    snprintf(buf, sizeof(buf), "%s/sys", rootfs->cr_target);

    err = umount2(buf, MNT_DETACH);
    if (err)
        LOG_INFO("conty_rootfs: skipping umount for %s", buf);

    err = mkdirat(rootfs->cr_dfd_mnt, "sys",
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (err < 0 && errno != EEXIST)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot create dentry %s", buf);

    err = mount("sysfs", buf, "sysfs", MS_NOEXEC | MS_NOSUID | MS_NODEV, NULL);
    if (err != 0)
        return LOG_ERROR_RET(-errno, "conty_rootfs: cannot mount sysfs at %s", buf);

    return err;
}