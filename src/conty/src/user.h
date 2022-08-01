#ifndef CONTY_USER_H
#define CONTY_USER_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define CONTY_USER_ID_MAP_MAX (sysconf(_SC_PAGESIZE))

struct conty_user_id_map {
    char  *buf;
    size_t cap;
    size_t written;
};

int conty_user_id_map_init(struct conty_user_id_map *m, char *buf, size_t buf_size);

int conty_user_id_map_put(struct conty_user_id_map *m, unsigned left,
                          unsigned right, unsigned range);

int conty_user_write_uid_mappings(pid_t pid, const struct conty_user_id_map *map);
int conty_user_write_gid_mappings(pid_t pid, const struct conty_user_id_map *map);

int conty_user_disable_setgroups(pid_t pid);

#endif //CONTY_USER_H
