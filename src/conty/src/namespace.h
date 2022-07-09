#ifndef CONTY_NAMESPACE_H
#define CONTY_NAMESPACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

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

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_NAMESPACE_H
