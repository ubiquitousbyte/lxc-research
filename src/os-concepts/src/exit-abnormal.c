#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/* Will get triggered when the caller pushes Ctrl+C */
void default_interaction_handler(int signal)
{
    printf("Terminating process\n");
    _exit(0);
}

int main(int argc, char *argv[])
{
    pid_t child = fork();
    if (child < 0)
        goto err;

    if (child == 0) {
        printf("Child identifier: %d\n", getpid());
        /* Child registers sigint handler */
        struct sigaction action = {.sa_handler = default_interaction_handler };
        if (sigaction(SIGINT, &action, NULL) != 0)
            goto err;

        /* Child is suspended until it receives a signal */
        pause();
    }

    /* Caller will block indefinitely until child exits */
    int status;
    if (waitpid(child, &status, 0) != child)
        goto err;

    printf("Child exited with status %d\n", status);

    return 0;
err:
    printf("%s", strerror(errno));
    return 1;
}