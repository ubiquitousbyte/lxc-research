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
    const char *crfs_source;
    const char *crfs_target;
    char        crfs_readonly;
};

int conty_rootfs_mount(const struct conty_rootfs *rootfs);

int conty_rootfs_pivot(const struct conty_rootfs *rootfs);

int conty_rootfs_mount_devfs(struct conty_rootfs *rootfs);

int conty_rootfs_mkdevices(struct conty_rootfs *rootfs);

int conty_rootfs_mount_procfs(const struct conty_rootfs *rootfs);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_MOUNT_H
