//
// Created by nas on 01/07/22.
//

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    printf("Parent: %d\n", getpid());

    pid_t child = fork();
    if (child < 0)
        goto err;

    if (child == 0) {
        printf("Child: %d\nChild parent: %d\n", getpid(), getppid());
    } else {
        int status;
        if (waitpid(child, &status, 0) != child)
            goto err;
    }

    return 0;
err:
    printf("%s", strerror(errno));
    return 1;
}