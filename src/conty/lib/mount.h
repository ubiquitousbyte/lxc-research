#ifndef CONTY_MOUNT_H
#define CONTY_MOUNT_H

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/*
 * Root filesystem of a container
 */
struct conty_rootfs {
    /*
     * Directory entry inside the container
     * where the root filesystem will be mounted
     */
    char cro_dst[PATH_MAX];
    /*
     * Local storage for path construction
     */
    char cro_buf[PATH_MAX];
    /*
     * Flag that indicates if the root filesystem should be writable
     */
    char cro_readonly;
};

int conty_rootfs_init(struct conty_rootfs *rfs, const char *dst, char readonly);

/*
 * Mounts the root filesystem
 */
int conty_rootfs_mount(const struct conty_rootfs *rfs);

/*
 * Creates a mount point under /dev in the root filesystem
 */
int conty_rootfs_mount_dev(struct conty_rootfs *rfs);

/*
 * Mounts a proc filesystem under the root filesystem's /proc directory
 */
int conty_rootfs_mount_proc(struct conty_rootfs *rfs);

/*
 * Mounts a sysfs filesystem under the root filesystem's /sys directory
 */
int conty_rootfs_mount_sys(struct conty_rootfs *rfs);

/*
 * Mount shm under /dev/shm in the new root filesystem to give
 * container applications the ability to create POSIX shared-memory objects
 */
int conty_rootfs_mount_shm(struct conty_rootfs *rfs);

/*
 * Mount mqueue under /dev/mqueue in the new root filesystem to give
 * container applications the ability to create POSIX message queueus
 */
int conty_rootfs_mount_mqueue(struct conty_rootfs *rfs);

/*
 * Creates all standard device nodes, e.g /dev/null, under the root
 * filesystem's /dev mount point
 */
int conty_rootfs_mkdev(struct conty_rootfs *rfs);

/*
 * Change the root node of the filesystem hierarchy to match the
 * destination entry of rfs
 *
 * Callers must mount the root filesystem via conty_rootfs_mount before pivoting
 */
int conty_rootfs_pivot(const struct conty_rootfs *rfs);

#endif //CONTY_MOUNT_H
