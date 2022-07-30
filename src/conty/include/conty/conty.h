#ifndef CONTY_CONTY_H
#define CONTY_CONTY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <unistd.h>

typedef enum {
    CONTY_SANDBOX_STATUS_CREATING,
    CONTY_SANDBOX_STATUS_CREATED,
    CONTY_SANDBOX_STATUS_RUNNING,
    CONTY_SANDBOX_STATUS_STOPPED,
} conty_sandbox_status;

struct conty_sandbox_state {
    char                *id;
    char                *bundle;
    conty_sandbox_status status;
};

struct conty_rt_ops {
    struct conty_sandbox_state *(*sandbox_state)(const char *id);
    int (*sandbox_create)(const char *id, const char *bundle);
    int (*sandbox_start)(const char *id);
    int (*sandbox_kill)(const char *id, int signal);
    int (*sandbox_delete)(const char *id);
};

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_CONTY_H
