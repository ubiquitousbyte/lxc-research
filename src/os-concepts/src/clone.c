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
    for (int i = 0; i < 25; i++)
        printf("%s", "This causes a protection fault\n");
    return 0;
}

int main(int argc, char *argv[])
{
    /*
     * The stack passed into clone must be aligned to a byte boundary
     * defined by the host system's ABI.
     * On ARM64 and x86_64 the boundary is 16 bytes, but we can make this
     * portable by using mmap which will align for us
     */
    size_t stack_len = sysconf(_SC_PAGESIZE)*2;
    int protection_flags = PROT_READ | PROT_WRITE;
    int configuration_flags = MAP_ANONYMOUS | MAP_SHARED;
    void *stack;

    stack = mmap(NULL, stack_len, protection_flags, configuration_flags, 0, 0);
    if (stack == MAP_FAILED)
        goto err;

    /*
     * Remember, the stack grows downwards and clone expects the top of the stack.
     * So we need to add stack_len to the base stack pointer to get to the top
     */
    pid_t child = clone(child_process, stack + stack_len, SIGCHLD, NULL);
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