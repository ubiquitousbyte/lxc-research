#include "user.h"

#include <fcntl.h>
#include <string.h>

#include "log.h"
#include "resource.h"
#include "safestring.h"

int conty_id_map_put(struct conty_id_map *map, unsigned int left,
                     unsigned int right, unsigned int range)
{
    int n_bytes;

    n_bytes = snprintf(NULL, 0, "%u %u %u\n", left, right, range);
    if (map->cidm_len + n_bytes > sizeof(map->cidm_buf) - 1)
        return log_error_ret(-EIO, "insufficient space in id map");

    n_bytes = strnprintf(&map->cidm_buf[map->cidm_len], n_bytes + 1, "%u %u %u\n",
                         left, right, range);
    if (n_bytes < 0)
        return log_error_ret(n_bytes, "insufficient space in id map");

    map->cidm_len += n_bytes;
    return 0;
}

static int conty_id_map_write_fd(const struct conty_id_map *map, int fd)
{
    if (fd <= 0)
        return -EBADF;

    if (write(fd, map->cidm_buf, map->cidm_len) != map->cidm_len)
        return log_error_ret(-errno, "could not write id map to file");

    return 0;
}

static int conty_id_map_write_path(const struct conty_id_map *map, const char *path)
{
    FD_RESOURCE int fd = -EBADF;

    fd = open(path, O_WRONLY);
    if (fd < 0)
        return -errno;

    return conty_id_map_write_fd(map, fd);
}

int conty_id_map_write_uids(const struct conty_id_map *map)
{
    return conty_id_map_write_path(map, "/proc/self/uid_map");
}

int conty_id_map_write_gids(const struct conty_id_map *map)
{
    return conty_id_map_write_path(map, "/proc/self/gid_map");
}

static int conty_id_map_write_ocids(const struct oci_ids *ids,
                                    int (*cb)(const struct conty_id_map *map))
{
    int err;
    struct oci_id_mapping *cur, *tmp;
    struct conty_id_map map;

    conty_id_map_init(&map);

    SLIST_FOREACH_SAFE(cur, ids, oid_next, tmp) {
        err = conty_id_map_put(&map, cur->oid_container, cur->oid_host, cur->oid_count);
        if (err != 0)
            return err;
    }

    return cb(&map);
}

int conty_id_map_write_oci_uids(const struct oci_ids *ids)
{
   return conty_id_map_write_ocids(ids, conty_id_map_write_uids);
}

int conty_id_map_write_oci_gids(const struct oci_ids *ids)
{
    return conty_id_map_write_ocids(ids, conty_id_map_write_gids);
}

int conty_id_disable_setgroups()
{
    FD_RESOURCE int fd = -EBADF;

    fd = open("/proc/self/setgroups", O_WRONLY | O_CLOEXEC);
    if (fd < 0)
        return -errno;

    char buf[] = "deny";
    if (write(fd, buf, sizeof(buf)) != sizeof(buf))
        return -1;

    return 0;
}
