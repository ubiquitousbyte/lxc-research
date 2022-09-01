#include <conty/conty.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    int err;
    int status;
    struct conty_container *cc;

    /*
     * Create the container with the configurations defined in net-config.json
     */
    cc = conty_container_create("fio", "io-config.json");
    if (!cc)
        return 1;

    if ((err = conty_container_start(cc)) != 0)
        goto cleanup;


    if (waitpid(conty_container_pid(cc), &status, 0) == conty_container_pid(cc)) {
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "container failed\n");
            err = -1;
        }
    } else
        err = -1;

cleanup:
    conty_container_set_status(cc, CONTY_STOPPED);
    conty_container_delete(cc);
    return err;
}
