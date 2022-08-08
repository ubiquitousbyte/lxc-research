#ifndef CONTY_USER_H
#define CONTY_USER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#define CONTY_USER_ID_MAP_MAX (sysconf(_SC_PAGESIZE))

struct conty_user_id_map {
    char  *buf;
    size_t cap;
    size_t written;
};

int conty_user_id_map_init(struct conty_user_id_map *m, char *buf, size_t buf_size);

int conty_user_id_map_put(struct conty_user_id_map *m, unsigned int left,
                          unsigned int right, unsigned int range);

static inline int conty_user_id_map_is_empty(const struct conty_user_id_map *map)
{
    return map->written == 0;
}

int __conty_user_write_mappings(const struct conty_user_id_map *map,
                               const char *path);

static inline int __conty_user_write_pid_mappings(const struct conty_user_id_map *map,
                                                  pid_t pid,
                                                  const char *map_name)
{
    char path[sizeof("/proc/xxxxxxxxxxxxxxxxxxx/uid_map")];
    sprintf(path, "/proc/%d/%s", pid, map_name);
    return __conty_user_write_mappings(map, path);
}

static inline int __conty_user_write_own_mappings(const struct conty_user_id_map *map,
                                                  const char *map_name)
{
    char path[sizeof("/proc/self/uid_map")];
    sprintf(path, "/proc/self/%s", map_name);
    return __conty_user_write_mappings(map, path);
}

static inline int conty_user_write_uid_mappings(const struct conty_user_id_map *map,
                                                pid_t pid)
{
    return __conty_user_write_pid_mappings(map, pid, "uid_map");
}

static inline int conty_user_write_gid_mappings(const struct conty_user_id_map *map,
                                                pid_t pid)
{
    return __conty_user_write_pid_mappings(map, pid, "gid_map");
}

static inline int conty_user_write_own_uid_mappings(const struct conty_user_id_map *map)
{
    return __conty_user_write_own_mappings(map, "uid_map");
}

static inline int conty_user_write_own_gid_mappings(const struct conty_user_id_map *map)
{
    return __conty_user_write_own_mappings(map, "gid_map");
}

int __conty_user_disable_setgroups(const char *path);

static inline int conty_user_disable_setgroups(pid_t pid)
{
    char path[sizeof("/proc/xxxxxxxxxxxxxxxxxxx/setgroups")];
    sprintf(path, "/proc/%d/setgroups", pid);
    return __conty_user_disable_setgroups(path);
}

static inline int conty_user_disable_own_setgroups()
{
    return __conty_user_disable_setgroups("/proc/self/setgroups");
}

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_USER_H
