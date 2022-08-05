#ifndef CONTY_MOUNT_H
#define CONTY_MOUNT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A bind mount
 */
struct conty_bind_mount {
    /*
     * The source directory on the host system
     */
    const char *source;
    /*
     * The target directory inside the mount namespace of the sandbox
     */
    const char *target;
    /*
     * File system event propagation type
     * Effectively determines the noninterference boundary between
     * filesystems in different sandboxes
     */
    unsigned long propagation;
    /*
     * Mount point attributes to set
     */
    unsigned long attr_set;
    /*
     * Mount point attributes to clear
     */
    unsigned long attr_clear;
};

/*
 * Creates the bind mount on the filesystem
 */
int conty_bind_mount_do(const struct conty_bind_mount *mnt);

struct conty_rootfs {
    /*
     * The bind mount that transforms an arbitrary directory holding
     * the root filesystem into a mount which can later be pivoted
     * by a sandbox
     */
    struct conty_bind_mount  mnt;
    /*
     * File descriptor to the mounted root filesystem
     */
    int                      dfd_mnt;
};

/*
 * Pivot changes the root mount in the mount namespace to rootfs
 */
int conty_rootfs_pivot(const struct conty_rootfs *rootfs);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_MOUNT_H
