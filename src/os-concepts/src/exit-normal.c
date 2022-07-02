#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

void exit_handler_1(void)
{
    printf("Process %d invoked exit handler 1\n", getpid());
}

void exit_handler_2(void)
{
    printf("Process %d invoked exit handler 2\n", getpid());
}

int main(int argc, char *argv[])
{
    printf("Parent process: %d\n", getpid());

    /* Register exit handlers in parent */
    atexit(exit_handler_1);
    atexit(exit_handler_2);

    pid_t child = fork();
    if (child < 0)
        goto err;

    if (child == 0) {
        /* Child process begins execution here */
        printf("Child process: %d\n", getpid());
        exit(0);
    }

    int status;
    if (waitpid(child, &status, 0) != child)
        goto err;

    printf("Child exited with status %d\n", status);

    return 0;
err:
    printf("%s", strerror(errno));
    return 1;
}