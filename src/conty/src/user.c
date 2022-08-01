#include "user.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "resource.h"

int conty_user_id_map_init(struct conty_user_id_map *m, char *buf, size_t buf_size)
{
    if (buf_size > CONTY_USER_ID_MAP_MAX)
        return -ENOSPC;

    m->buf = buf;
    m->cap = buf_size;
    m->written = 0;

    return 0;
}

int conty_user_id_map_put(struct conty_user_id_map *m, unsigned left,
                          unsigned right, unsigned range)
{
    if (range < 1)
        return -1;

    size_t spc = snprintf(NULL, 0, "%u %u %u\n", left, right, range);
    if (m->written + spc > m->cap)
        return -1;

    sprintf(&m->buf[m->written], "%u %u %u\n", left, right, range);
    m->written += spc;

    return 0;
}

static int __conty_user_write_mappings(pid_t pid,
                                       const char *map_name,
                                       const struct conty_user_id_map *map)
{
    __CONTY_CLOSE int mfd = -EBADF;
    char path[sizeof("/proc/xxxxxxxxxxxxxxxxxxx/uid_map")];

    sprintf(path, "/proc/%d/%s", pid, map_name);

    mfd = open(path, O_WRONLY | O_CLOEXEC);
    if (mfd < 0)
        return -1;

    if (write(mfd, map->buf, map->written) != map->written)
        return -1;

    return 0;
}

int conty_user_write_uid_mappings(pid_t pid, const struct conty_user_id_map *map)
{
    return __conty_user_write_mappings(pid, "uid_map", map);
}

int conty_user_write_gid_mappings(pid_t pid, const struct conty_user_id_map *map)
{
    return __conty_user_write_mappings(pid, "gid_map", map);
}

int conty_user_disable_setgroups(pid_t pid)
{
    __CONTY_CLOSE int gfd = -EBADF;
    char path[sizeof("/proc/xxxxxxxxxxxxxxxxxxx/setgroups")];

    sprintf(path, "/proc/%d/setgroups", pid);

    gfd = open(path, O_WRONLY | O_CLOEXEC);
    if (gfd < 0)
        return -1;

    char buf[] = "deny";
    if (write(gfd, buf, sizeof(buf)) != sizeof(buf))
        return -1;

    return 0;
}