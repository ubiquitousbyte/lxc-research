#include <conty/conty.h>
#include "user.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include "resource.h"

int conty_id_map_put(struct conty_id_map *map, unsigned int left,
                     unsigned int right, unsigned int range)
{
    int err;

    size_t n_bytes = snprintf(NULL, 0, "%u %u %u\n", left, right, range);
    if (map->cim_len + n_bytes > sizeof(map->cim_buf) - 1)
        return -EIO;

    err = CONTY_SNPRINTF(&map->cim_buf[map->cim_len], n_bytes + 1,
                         "%u %u %u\n", left, right, range);
    if (err < 0)
        return err;

    map->cim_len += n_bytes;

    return 0;
}

int conty_id_map_write_fd(const struct conty_id_map *map, int fd)
{
    if (fd <= 0)
        return -EBADF;

    if (write(fd, map->cim_buf, map->cim_len) != map->cim_len)
        return -1;

    return 0;
}

int conty_id_map_write_path(const struct conty_id_map *map, const char *path)
{
    __CONTY_CLOSE int fd = -EBADF;

    fd = open(path, O_WRONLY);
    if (fd < 0)
        return -errno;

    return conty_id_map_write_fd(map, fd);
}

int conty_oci_write_id_map(const struct oci_uids *uids, const char *path)
{
    int err;
    struct oci_id_mapping *cur, *tmp;
    struct conty_id_map map;
    conty_id_map_init(&map);

    LIST_FOREACH_SAFE(cur, uids, oid_next, tmp) {
        err = conty_id_map_put(&map, cur->oid_container,
                               cur->oid_host, cur->oid_count);
        if (err != 0)
            return err;
    }

    return conty_id_map_write_path(&map, path);
}

int conty_oci_write_uid_map(const struct oci_uids *uids)
{
    return conty_oci_write_id_map(uids, "/proc/self/uid_map");
}

int conty_oci_write_gid_map(const struct oci_uids *gids)
{
    return conty_oci_write_id_map(gids, "/proc/self/gid_map");
}

int conty_usr_deny_setgroups()
{
    __CONTY_CLOSE int fd = -EBADF;

    fd = open("/proc/self/setgroups", O_WRONLY | O_CLOEXEC);
    if (fd < 0)
        return -errno;

    char buf[] = "deny";
    if (write(fd, buf, sizeof(buf)) != sizeof(buf))
        return -1;

    return 0;
}