#ifndef CONTY_USER_H
#define CONTY_USER_H

#include <stddef.h>
#include <string.h>

#include "oci.h"

#define CONTY_ID_MAP_SIZE 4096

/*
 * In-memory identifier mapping
 */
struct conty_id_map {
    char    cidm_buf[CONTY_ID_MAP_SIZE];
    size_t  cidm_len;
};

static inline void conty_id_map_init(struct conty_id_map *map)
{
    memset(map->cidm_buf, 0, CONTY_ID_MAP_SIZE);
    map->cidm_len = 0;
}

/*
 * Put an id mapping in the map
 */
int conty_id_map_put(struct conty_id_map *map, unsigned int left,
                     unsigned int right, unsigned int range);

/*
 * Dump the identifier mappings in the map into the process's uid_map file
 */
int conty_id_map_write_uids(const struct conty_id_map *map);

/*
 * Dump the identifier mappings in the map into the process's gid_map file
 */
int conty_id_map_write_gids(const struct conty_id_map *map);

/*
 * Dump the OCI identifier mappings into the process's uid_map file
 */
int conty_id_map_write_oci_uids(const struct oci_ids *ids);

/*
 * Dump the OCI identifier mappings into the process's gid_map file
 */
int conty_id_map_write_oci_gids(const struct oci_ids *ids);

/*
 * Disallow usage of the setgroups system call
 */
int conty_id_disable_setgroups();

#endif //CONTY_USER_H
