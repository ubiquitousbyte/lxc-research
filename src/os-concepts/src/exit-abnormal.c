#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

void default_interaction_handler(int signal)
{
    /* Will get triggered when the caller pushes Ctrl+C */
    printf("Terminating process\n");
    _exit(0);
}

int main(int argc, char *argv[])
{
    struct sigaction action = {.sa_handler = default_interaction_handler };
    if (sigaction(SIGINT, &action, NULL) != 0)
        goto err;

    /* Suspend process */
    for ( ;; )
        pause();

err:
    printf("%s", strerror(errno));
}