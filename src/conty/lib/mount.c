#include "mount.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "resource.h"
#include "log.h"
#include "safestring.h"

#define OPENDIR_FLAGS (O_DIRECTORY | O_PATH | O_CLOEXEC | O_NOFOLLOW)

/*
 * Atomically swaps the old root mount point of the current process
 * with new_root and places the old contents in put_old
 *
 * This operation also changes the root directory of every active process
 * to new_root in the same mount namespace
 */
static inline int pivot_root(const char *new_root, const char *put_old)
{
    return (int) syscall(SYS_pivot_root, new_root, put_old);
}

int conty_rootfs_init(struct conty_rootfs *rfs, const char *dst, char readonly)
{
    int err;

    err = strnprintf(rfs->cro_dst, sizeof(rfs->cro_dst), "%s", dst);
    if (err < 0)
        return log_error_ret(err, "could not construct rootfs destination path");

    memset(rfs->cro_buf, 0, sizeof(rfs->cro_buf));
    rfs->cro_readonly = readonly;

    return 0;
}

int conty_rootfs_pivot(const struct conty_rootfs *rfs)
{
    LOG_INFO("pivoting rootfs %s", rfs->cro_dst);

    FD_RESOURCE int old_root = -EBADF, new_root = -EBADF;

    if ((new_root = openat(-EBADF, rfs->cro_dst, OPENDIR_FLAGS)) < 0)
        return log_error_ret(-errno, "cannot open %s", rfs->cro_dst);

    if ((old_root = openat(-EBADF, "/", OPENDIR_FLAGS)) < 0)
        return log_error_ret(-errno, "cannot open /");

    /*
     * Move the current process to where its new root filesystem is
     */
    if (fchdir(new_root) < 0)
        return log_error_ret(-errno, "cannot chdir into %s", rfs->cro_dst);

    /*
     * Swap the old root mount point with the new one.
     *
     * This operation also overlays the old root on top of the old one.
     * This avoids us having to create a temporary directory for the old
     * root filesystem. This has weird implications on how "/" is interpreted
     * after the operation, e.g:
     *      $ pivot_root . .
     *      $ cd ..
     *      ## /
     * This operation puts us in old_root from new_root, but then if we do
     *      $ cd /
     *  we'll end up back in the new root
     *
     *  It is therefore crucial to unmount the old root after pivoting
     */
    if (pivot_root(".", ".") < 0)
        return log_error_ret(-errno, "pivot root failed");

    if (fchdir(old_root) < 0)
        return log_error_ret(-errno, "cannot unmount old root");

    /*
     * The old root filesystem may share mount propagation events
     * with the newly created mount namespace. if we unmount it, the event
     * may be sent to the host, which we don't want, so
     * reconfigure the old root filesystem to not propagate events before umount
     */
    if (mount("", ".", "", MS_SLAVE | MS_REC, NULL) < 0)
        return log_error_ret(-errno, "cannot unmount old root");

    /*
     * Now it should be safe to unmount
     */
    if (umount2(".", MNT_DETACH) < 0)
        return log_error_ret(-errno, "cannot unmount old root");

    /*
     * pivot_root does not switch to the new root directory, we do it manually
     */
    if (fchdir(new_root) < 0)
        return log_error_ret(-errno, "cannot chdir into new root");

    return 0;
}

int conty_rootfs_mount(const struct conty_rootfs *rfs)
{
    /*
     * Reconfigure the root file system as private so that
     * the bind mount of the new root filesystem does not trigger a mount
     * event in any other namespaces
     */
    if (mount("", "/", "", MS_PRIVATE | MS_REC, NULL) != 0)
        return log_error_ret(-errno, "could not mount --make-rslave /");

    /*
     * Convert the dentry holding the root filesystem into a mount point
     */
    unsigned long mflags = MS_BIND | MS_REC;
    if (mount(rfs->cro_dst, rfs->cro_dst, "bind", mflags, NULL) != 0)
        return log_error_ret(-errno, "could not mount --rbind %s", rfs->cro_dst);

    /*
     * Make sure that the new mount point also does not trigger any events
     * in other mount namespaces
     */
    if (mount("", rfs->cro_dst, "", MS_PRIVATE, NULL) != 0)
        return log_error_ret(-errno, "could not mount --make-private %s", rfs->cro_dst);

    return 0;
}

int conty_rootfs_mount_dev(struct conty_rootfs *rfs)
{
    LOG_INFO("preparing dev at %s/dev", rfs->cro_dst);

    int err;
    mode_t procmask;

    err = strnprintf(rfs->cro_buf, sizeof(rfs->cro_buf), "%s/dev", rfs->cro_dst);
    if (err < 0)
        return log_error_ret(err, "cannot construct path %s/dev", rfs->cro_dst);

    /*
     * Turn off execute permissions for the directory when we create it
     */
    procmask = umask(S_IXUSR | S_IXGRP | S_IXOTH);
    err = mkdir(rfs->cro_buf, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (err < 0 && errno != EEXIST) {
        LOG_ERROR("failed to create dev directory");
        err = -errno;
        goto fix_procmask;
    }

    /*
     * Mount a temporary filesystem inside the newly created directory
     * Tmpfs is, by default, world-writable which we don't want, so we
     * set the mode to 0755 to make it writable only by the current user
     * and limit its capacity to hold data
     */
    err = mount("none", rfs->cro_buf, "tmpfs", 0, "mode=0755,size=500000");
    if (err < 0) {
        LOG_ERROR("failed to mount dev directory: %s", strerror(errno));
        err = -errno;
    }

fix_procmask:
    umask(procmask);
    LOG_INFO("prepared dev directory");
    return err;
}

int conty_rootfs_mkdev(struct conty_rootfs *rfs)
{
    LOG_INFO("setting up device nodes");

    static const struct {
        const char *name;
        const mode_t mode;
        const int major;
        const int minor;
    } devs[6] = {
            { "null",    S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO, 1, 3 },
            { "zero",    S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO, 1, 5 },
            { "full",    S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO, 1, 7 },
            { "random",  S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO, 1, 8 },
            { "urandom", S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO, 1, 9 },
            { "tty",     S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO, 5, 0 },
    };
    int err;
    char hostdev[PATH_MAX];
    mode_t devmode, procmask;

    /*
     * Disable execution privileges when creating new inodes from this point on
     */
    procmask = umask(S_IXUSR | S_IXGRP | S_IXOTH);
    for (int i = 0; i < sizeof(devs) / sizeof(devs[0]); i++) {
        err = strnprintf(rfs->cro_buf, sizeof(rfs->cro_buf),
                         "%s/dev/%s", rfs->cro_dst, devs[i].name);
        if (err < 0) {
            LOG_ERROR("failed to construct device path to %s", devs[i].name);
            goto fix_procmask;
        }

        /*
         * Try to create an isolated device instance in dev and if
         * that does not work due to permission errors, resort to
         * bind mounting the host device into the container
         */
        devmode = makedev(devs[i].major, devs[i].minor);
        err = mknod(rfs->cro_buf, devs[i].mode, devmode);
        if (err < 0 && errno == EPERM) {
            LOG_DEBUG("failed to create device %s", devs[i].name);

            err = strnprintf(hostdev, sizeof(hostdev), "/dev/%s", devs[i].name);
            if (err < 0) {
                LOG_ERROR("failed to construct host device path %s", devs[i].name);
                goto fix_procmask;
            }

            FD_RESOURCE int fd = -EBADF;

            fd = open(rfs->cro_buf, O_CREAT | O_CLOEXEC);
            if (fd < 0 && errno != EEXIST) {
                err = -errno;
                LOG_ERROR("cannot bind mount device at %s", rfs->cro_buf);
                goto fix_procmask;
            }

            err = mount(hostdev, rfs->cro_buf, NULL, MS_BIND, NULL);
            if (err != 0) {
                err = -errno;
                LOG_ERROR("cannot bind mount device at %s", rfs->cro_buf);
                goto fix_procmask;
            }

            LOG_DEBUG("bind mounted %s at %s", devs[i].name, rfs->cro_buf);
        }
    }

    LOG_INFO("finished setting up device nodes");

fix_procmask:
    umask(procmask);
    return err;
}

static int conty_rootfs_mount_pseudofs(struct conty_rootfs *rfs, const char *name,
                                       const char *fstype, unsigned int flags,
                                       mode_t perm)
{
    int err;

    err = strnprintf(rfs->cro_buf, sizeof(rfs->cro_buf), "%s/%s", rfs->cro_dst, name);
    if (err < 0)
        return log_error_ret(err, "failed to create path to %s", name);

    err = mkdir(rfs->cro_buf, perm);
    if (err < 0 && errno != EEXIST)
        return log_error_ret(err, "failed to mkdir %s directory", name);

    err = mount("", rfs->cro_buf, fstype, flags, NULL);
    if (err < 0)
        return log_error_ret(-errno, "failed to mount %s", fstype);

    err = mount("", rfs->cro_buf, "", MS_PRIVATE, NULL);
    if (err < 0)
        return log_error_ret(-errno, "failed to make %s private", rfs->cro_buf);

    return err;
}

int conty_rootfs_mount_proc(struct conty_rootfs *rfs)
{
    LOG_INFO("mounting proc at %s/proc", rfs->cro_dst);
    unsigned int flags = MS_NODEV | MS_NOSUID | MS_NOEXEC | MS_REC;
    mode_t perm = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    return conty_rootfs_mount_pseudofs(rfs, "proc", "proc", flags, perm);
}

int conty_rootfs_mount_sys(struct conty_rootfs *rfs)
{
    LOG_INFO("mounting sysfs at %s/sys", rfs->cro_dst);
    unsigned int flags = MS_NODEV | MS_NOSUID | MS_NOEXEC | MS_REC;
    mode_t perm = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    return conty_rootfs_mount_pseudofs(rfs, "sys", "sysfs", flags, perm);
}

int conty_rootfs_mount_mqueue(struct conty_rootfs *rfs)
{
    LOG_INFO("mounting mqueue at %s/dev/mqueue", rfs->cro_dst);
    unsigned int flags = MS_NODEV | MS_NOSUID | MS_NOEXEC;
    mode_t perm =  S_IRWXU | S_IRWXG | S_IRWXO;
    return conty_rootfs_mount_pseudofs(rfs, "dev/mqueue", "mqueue", flags, perm);
}

int conty_rootfs_mount_shm(struct conty_rootfs *rfs)
{
    LOG_INFO("mounting shm at %s/dev/shm", rfs->cro_dst);
    /*
     * shm can't be mounted with MS_NOEXEC because
     * it may break some apps that mmap a shared-memory region with PROT_EXEC set
     */
    unsigned int flags = MS_NODEV | MS_NOSUID;
    /*
     * We want regular users to be able to create shm objects
     * so the permissions on this mount point are pretty liberal
     */
    mode_t perm =  S_IRWXU | S_IRWXG | S_IRWXO;
    return conty_rootfs_mount_pseudofs(rfs, "dev/shm", "tmpfs", flags, perm);
}