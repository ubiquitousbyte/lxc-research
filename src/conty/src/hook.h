//
// Created by nas on 29/07/22.
//

#ifndef CONTY_HOOK_H
#define CONTY_HOOK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <unistd.h>

#include "queue.h"

struct conty_hook {
    const char             *prog;
    const char            **argv;
    const char            **envp;
    int                     timeout;
    TAILQ_ENTRY(conty_hook) next;
};

int conty_hook_exec(const struct conty_hook *hook, const char *buf, size_t buf_len);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_HOOK_H
