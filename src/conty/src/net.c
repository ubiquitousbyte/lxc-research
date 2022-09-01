#include <conty/conty.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int err;
    int status;
    struct conty_container *cc;

    /*
     * Create the container with the configurations defined in net-config.json
     */
    cc = conty_container_create("iperf-server", "net-config.json");
    if (!cc)
        return 1;

    /*
     * Start the container, i.e the iperf3 server
     */
    if ((err = conty_container_start(cc)) != 0)
        goto cleanup;

    /*
     * We'll also need to start the iperf3 client, so fork a child and do that
     */
    pid_t child = fork();
    if (child < 0) {
        err = -1;
        goto kill_and_cleanup;
    }

    if (child == 0) {
        sleep(1);
        execlp("iperf3", "iperf3", "--logfile", "test.iperf3",
               "-J", "-c", "192.168.168.2", (char *) NULL);
        _exit(1);
    }

    if (waitpid(child, &status, 0) == child) {
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "iperf3 client failed\n");
            err = -1;
        }
    }

kill_and_cleanup:
    /*
     * Kill the container and delete all resources
     */
    conty_container_kill(cc, SIGTERM);
cleanup:
    conty_container_set_status(cc, CONTY_STOPPED);
    conty_container_delete(cc);
    return err;
}
