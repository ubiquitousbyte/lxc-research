#ifndef CONTY_CLONE_H
#define CONTY_CLONE_H

#include <unistd.h>

pid_t conty_clone(int (*fn)(void *), void *arg, int flags, int *pidfd);

pid_t conty_clone3(unsigned long flags, int *pidfd);

pid_t conty_clone3_cb(int (*fn)(void*), void *arg, unsigned long flags, int *pidfd);

int conty_clone_wait_exited(pid_t pid, int *status);

#endif //CONTY_CLONE_H
