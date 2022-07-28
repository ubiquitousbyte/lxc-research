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
    int                                           timeout;
};

int conty_hook_init(struct conty_hook *hook, const char *path);
void conty_hook_put_arg(struct conty_hook *hook, struct conty_hook_param *arg);
void conty_hook_put_env(struct conty_hook *hook, struct conty_hook_param *env);
int conty_hook_put_timeout(struct conty_hook *hook, int timeout);
int conty_hook_exec(struct conty_hook *hook, const char *buf, size_t buf_len);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_CONTY_H
