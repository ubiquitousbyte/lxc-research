#ifndef CONTY_NAMESPACE_H
#define CONTY_NAMESPACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

typedef enum {
    CONTY_NS_CGROUP,
    CONTY_NS_IPC,
    CONTY_NS_NET,
    CONTY_NS_MOUNT,
    CONTY_NS_PID,
    CONTY_NS_TIME,
    CONTY_NS_USER,
    CONTY_NS_UTS,
} conty_ns_type;

#define CONTY_NS_MAX (CONTY_NS_UTS + 1)

struct conty_ns;

struct conty_ns *conty_ns_open(pid_t pid, conty_ns_type type);

struct conty_ns *conty_ns_open_current(conty_ns_type type);

ino_t conty_ns_inode(const struct conty_ns *ns);

dev_t conty_ns_device(const struct conty_ns *ns);

int conty_ns_is(const struct conty_ns *left, const struct conty_ns *right);

int conty_ns_join(const struct conty_ns *ns);

int conty_ns_detach(int flags);

struct conty_ns *conty_ns_parent(const struct conty_ns *ns);

void conty_ns_close(struct conty_ns *ns);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_NAMESPACE_H
