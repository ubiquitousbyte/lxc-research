#ifndef CONTY_USER_H
#define CONTY_USER_H

#include <string.h>

#define CONTY_ID_MAP_BUF_SIZE 4096

/*
 * An in-memory identifier map that buffers uid/gid mappings in memory
 */
struct conty_id_map {
    /*
     * In-memory buffer
     * The kernel typically restricts the amount of data that can be dumped
     * into a id map file to a page size, but we cap this at 4 kb
     */
    char   cim_buf[CONTY_ID_MAP_BUF_SIZE];
    /*
     * The amount of data written to the buffer
     */
    size_t cim_len;
};

/*
 * Initialise the identifier map
 */
static inline void conty_id_map_init(struct conty_id_map *map)
{
    memset(map->cim_buf, 0, sizeof(map->cim_buf));
    map->cim_len = 0;
}

/*
 * Put an identifier mapping left:right:range that maps the identifier range
 * [left, left+range] on the host to [right, right+range] inside the container
 */
int conty_id_map_put(struct conty_id_map *map, unsigned int left,
                     unsigned int right, unsigned int range);

/*
 * Returns 1 if the identifierm ap is empty, 0 if not
 */
static inline int conty_id_map_is_empty(const struct conty_id_map *map)
{
    return map->cim_len == 0;
}

/*
 * Dumps the identifier map to file specified by the file descriptor
 */
int conty_id_map_write_fd(const struct conty_id_map *map, int fd);

/*
 * Small utility that denies usage of the setgroups call
 * This needs to be called before writing a gid map inside a container,
 * otherwise the process would be able to call setgroups and manipulate
 * its group mappings after the fact, which is a security risk
 */
int conty_usr_deny_setgroups();

#endif //CONTY_USER_H
