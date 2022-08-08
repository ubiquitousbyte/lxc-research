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

    memset(buf, 0, buf_size);

    m->buf = buf;
    m->cap = buf_size;
    m->written = 0;

    return 0;
}

int conty_user_id_map_put(struct conty_user_id_map *m, unsigned int left,
                          unsigned int right, unsigned int range)
{
    if (range < 1)
        return -EINVAL;

    size_t spc = snprintf(NULL, 0, "%u %u %u\n", left, right, range);
    if (m->written + spc > m->cap)
        return -ENOSPC;

    sprintf(&m->buf[m->written], "%u %u %u\n", left, right, range);
    m->written += spc;

    return 0;
}

int __conty_user_write_mappings(const struct conty_user_id_map *map,
                                const char *path)
{
    __CONTY_CLOSE int mfd = -EBADF;

    mfd = open(path, O_WRONLY | O_CLOEXEC);
    if (mfd < 0)
        return -EINVAL;

    if (write(mfd, map->buf, map->written) != map->written)
        return -errno;

    return 0;
}

int __conty_user_disable_setgroups(const char *path)
{
    __CONTY_CLOSE int gfd = -EBADF;

    gfd = open(path, O_WRONLY | O_CLOEXEC);
    if (gfd < 0)
        return -errno;

    char buf[] = "deny";
    if (write(gfd, buf, sizeof(buf)) != sizeof(buf))
        return -errno;

    return 0;
}