#ifndef CONTY_USER_H
#define CONTY_USER_H

#include <string.h>

#define CONTY_ID_MAP_BUF_SIZE 4096

struct conty_id_map {
    char   cim_buf[CONTY_ID_MAP_BUF_SIZE];
    size_t cim_len;
};

static inline void conty_id_map_init(struct conty_id_map *map)
{
    memset(map->cim_buf, 0, sizeof(map->cim_buf));
    map->cim_len = 0;
}

int conty_id_map_put(struct conty_id_map *map, unsigned int left,
                     unsigned int right, unsigned int range);

static inline int conty_id_map_is_empty(const struct conty_id_map *map)
{
    return map->cim_len == 0;
}

int conty_id_map_write_fd(const struct conty_id_map *map, int fd);

int conty_usr_deny_setgroups();

#endif //CONTY_USER_H
