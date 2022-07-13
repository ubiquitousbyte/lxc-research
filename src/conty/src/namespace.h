#ifndef CONTY_NAMESPACE_H
#define CONTY_NAMESPACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <sys/types.h>

#include <unistd.h>

struct conty_ns;

struct conty_ns *conty_ns_open(pid_t pid, int type);

struct conty_ns *conty_ns_open_current(int type);

struct conty_ns *conty_ns_from_fd(int fd);

ino_t conty_ns_inode(const struct conty_ns *ns);

dev_t conty_ns_device(const struct conty_ns *ns);

int conty_ns_is(const struct conty_ns *left, const struct conty_ns *right);

int conty_ns_join(const struct conty_ns *ns);

int conty_ns_detach(int namespaces);

struct conty_ns *conty_ns_parent(const struct conty_ns *ns);

void conty_ns_close(struct conty_ns *ns);

/*
 * Max number of bytes to write to /proc/[pid]/uid_map
 */
#define CONTY_NS_ID_MAP_MAX (sysconf(_SC_PAGESIZE))

struct conty_ns_id_map {
    char  *buf;
    size_t cap;
    size_t written;
};

int conty_ns_id_map_init(struct conty_ns_id_map *m, char *buf, size_t buf_size);

int conty_ns_id_map_put(struct conty_ns_id_map *m, int left, int right, int range);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_NAMESPACE_H
