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

/*
 * Removes the bind mount from the filesystem
 */
int conty_bind_mount_undo(const struct conty_bind_mount *mnt);

struct conty_rootfs {
    struct conty_bind_mount *mnt;
    int                      dfd_mnt;
};

/*
 * Mounts all pseudo filesystems at their default destinations
 * relative to a root filesystem
 */
int conty_rootfs_mount_pseudofs(const struct conty_rootfs *rootfs);

int conty_rootfs_pivot(const struct conty_rootfs *rootfs);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_MOUNT_H
