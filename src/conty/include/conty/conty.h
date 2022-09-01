#ifndef CONTY_CONTY_H
#define CONTY_CONTY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

typedef enum {
    CONTY_CREATING = 0,
    CONTY_CREATED  = 1,
    CONTY_RUNNING  = 2,
    CONTY_STOPPED  = 3
} conty_container_status_t;

struct conty_container;
struct conty_container *conty_container_create(const char *id, const char *bundle);
int conty_container_start(struct conty_container *container);
int conty_container_kill(struct conty_container *container, int sig);
int conty_container_delete(struct conty_container *container);
void conty_container_free(struct conty_container *container);
int conty_container_pollfd(const struct conty_container *cc);
pid_t conty_container_pid(const struct conty_container *cc);
void conty_container_set_status(struct conty_container *cc,
                                conty_container_status_t status);
conty_container_status_t conty_container_status(const struct conty_container *container);
const char *conty_container_status_str(const struct conty_container *container);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_CONTY_H
