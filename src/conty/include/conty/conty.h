#ifndef CONTY_CONTY_H
#define CONTY_CONTY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <conty/queue.h>

struct conty_hook_param {
    const char *param;
    SLIST_ENTRY(conty_hook_param) next;
};

struct conty_hook {
    const char                                   *path;
    SLIST_HEAD(conty_hook_argv, conty_hook_param) args;
    SLIST_HEAD(conty_hook_env, conty_hook_param)  envp;
    size_t                                        args_count;
    size_t                                        env_count;
    int                                           timeout_ms;
};

int conty_hook_init(struct conty_hook *hook, const char *path);
void conty_hook_put_arg(struct conty_hook *hook, struct conty_hook_param *arg);
void conty_hook_put_env(struct conty_hook *hook, struct conty_hook_param *env);
int conty_hook_put_timeout(struct conty_hook *hook, int timeout);
int conty_hook_exec(struct conty_hook *hook, const char *buf,
                    size_t buf_len, int *status);

struct conty_event_hooks {
    SLIST_HEAD(conty_on_rt_create_hooks, conty_hook)    on_rt_create;
    SLIST_HEAD(conty_on_cont_created_hooks, conty_hook) on_cont_created;
    SLIST_HEAD(conty_on_cont_start_hooks, conty_hook)   on_cont_start;
    SLIST_HEAD(conty_on_cont_started_hooks, conty_hook) on_cont_started;
    SLIST_HEAD(conty_on_cont_stopped_hooks, conty_hook) on_cont_stopped;
};

int conty_event_hooks_init(struct conty_event_hooks *hooks);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_CONTY_H
