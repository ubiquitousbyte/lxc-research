#ifndef CONTY_MOUNT_H
#define CONTY_MOUNT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

#include "resource.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

struct conty_rootfs {
    char *crfs_source;
    char *crfs_target;
    char  crfs_readonly;
    char  crfs_buf[PATH_MAX];
};

int conty_rootfs_mount(const struct conty_rootfs *rootfs);

int conty_rootfs_pivot(const struct conty_rootfs *rootfs);

int conty_rootfs_mount_devfs(struct conty_rootfs *rootfs);

int conty_rootfs_mkdevices(struct conty_rootfs *rootfs);

int conty_rootfs_mount_procfs(struct conty_rootfs *rootfs);

int conty_rootfs_mount_sysfs(struct conty_rootfs *rootfs);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_MOUNT_H
