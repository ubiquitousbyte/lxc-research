#ifndef CONTY_CONTY_H
#define CONTY_CONTY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CONTY_CREATING,
    CONTY_CREATED,
    CONTY_RUNNING,
    CONTY_STOPPED
} conty_container_status_t;

#define CONTY_STATUS_MAX (CONTY_STOPPED)

struct conty_container;
struct conty_container *conty_container_create(const char *id, const char *bundle);
int conty_container_start(struct conty_container *container);
int conty_container_kill(struct conty_container *container, int sig);
int conty_container_delete(struct conty_container *container);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_CONTY_H
