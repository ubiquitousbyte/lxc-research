#ifndef CONTY_MOUNT_H
#define CONTY_MOUNT_H

#ifdef __cplusplus
extern "C" {
#endif

struct conty_rootfs {
    const char              *cr_source;
    const char              *cr_target;
    char                     cr_ro;
    int                      cr_dfd_mnt;
};

void conty_rootfs_init_runtime(struct conty_rootfs *rootfs, const char *source,
                              const char *target, char readonly);

int conty_rootfs_init_container(struct conty_rootfs *rootfs);

int conty_rootfs_mount(const struct conty_rootfs *rootfs);

/*
 * Pivot changes the root mount in the mount namespace to rootfs
 */
int conty_rootfs_pivot(const struct conty_rootfs *rootfs);

int conty_rootfs_mount_devices(const struct conty_rootfs *rootfs);

int conty_rootfs_mount_proc(const struct conty_rootfs *rootfs);

int conty_rootfs_mount_sys(const struct conty_rootfs *rootfs);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_MOUNT_H
