//
// Created by nas on 29/07/22.
//

#ifndef CONTY_HOOK_H
#define CONTY_HOOK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

int conty_hook_exec(const char *prog, const char *argv[], const char *envp[],
                    const char *buf, size_t buf_len, int timeout);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_HOOK_H
