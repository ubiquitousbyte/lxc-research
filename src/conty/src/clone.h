#ifndef CONTY_CLONE_H
#define CONTY_CLONE_H

#include <unistd.h>
#include <linux/types.h>

/*
 * This article helped here: https://lkml.org/lkml/2019/10/25/184
 */
#ifndef ptr_to_u64
#define ptr_to_u64(ptr) ((__u64)((uintptr_t)(ptr)))
#endif
#ifndef u64_to_ptr
#define u64_to_ptr(x) ((void *)(uintptr_t)x)
#endif

pid_t conty_clone3(unsigned long flags, int *pidfd, void *stack, size_t stack_size);

int conty_clone3_cb(int (*fn)(void*), void *arg, unsigned long flags, int *pidfd,
                    void *stack, size_t stack_size);

#endif //CONTY_CLONE_H
