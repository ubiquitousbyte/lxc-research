//
// Created by nas on 01/07/22.
//

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    pid_t child = fork();
    switch (child) {
    case 0:
        printf("Child: %d\nChild parent: %d\n", getpid(), getppid());
        break;
    case -1:
        printf("%s", strerror(errno));
        return 1;
    default:
        printf("Parent: %d\n", getpid());
    }
}