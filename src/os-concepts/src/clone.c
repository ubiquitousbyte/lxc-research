/* Needs to be defined in order to use clone */
#define _GNU_SOURCE

#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

/* The entry point into the child  */
int child_process(void *data)
{
    printf("Child: %d\n", getpid());
    return 0;
}

int main(int argc, char *argv[])
{
    printf("Parent: %d\n", getpid());

    size_t stack_len = sysconf(_SC_PAGESIZE)*2;
    int protection_flags = PROT_READ | PROT_WRITE;
    int configuration_flags = MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK;
    void *stack = mmap(NULL, stack_len, protection_flags,
                       configuration_flags, 0, 0);
    if (stack == MAP_FAILED)
        goto err;

    /* The parent will share nothing with the child, because no flags are set */
    int shared_resources = 0;
    pid_t child = clone(child_process, stack + stack_len,
                        shared_resources | SIGCHLD, NULL);
    if (child == -1)
        goto err;

    int status;
    if (waitpid(child, &status, 0) != child)
        goto err;

    return 0;
err:
    printf("%s", strerror(errno));
    return 1;
}