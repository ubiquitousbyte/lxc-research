#include <conty/conty.h>

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/signalfd.h>

#include <argp.h>

#ifndef PATH_MAX
#def PATH_MAX 4096
#endif

#define CONTY_CONTFD 0
#define CONTY_SIGFD 1

static char doc[] = "conty-runner -- A program that runs containers";

const char *argp_program_bug_address = "htw-berlin.de";
const char *argp_program_version = "version 1.0";

struct conty_args {
    long        ca_timeout;
    char        ca_bundle[PATH_MAX];
    const char *ca_name;
};

struct argp_option conty_options[] = {
        {
            "bundle",
            'b',
            "PATH",
            0,
            "Path to container bundle"
        },
        {
            "timeout",
            't',
            "NUMBER",
            OPTION_ARG_OPTIONAL,
            "Time to wait for container to exit before killing"
        },
        { 0 },
};

static int conty_parse_opt(int key, char *arg, struct argp_state *state)
{
    struct conty_args *args = (struct conty_args *) state->input;

    switch (key) {
    case 'b':
        if (!realpath(arg, args->ca_bundle))
            argp_error(state, "invalid bundle path: %s", strerror(errno));

        if (access(args->ca_bundle, R_OK) != 0)
            argp_error(state, "cannot access bundle: %s", strerror(errno));

        break;
    case 't':
        if (arg) {
            char *ptr;
            args->ca_timeout = strtol(arg, &ptr, 10);
            if (errno == ERANGE)
                argp_error(state, "invalid timeout: %s", strerror(ERANGE));

            if (ptr == arg)
                argp_error(state, "invalid timeout: no digits found");

            if (args->ca_timeout < 0)
                args->ca_timeout = -1;
            else
                args->ca_timeout *= 1000;
        }
        break;
    case ARGP_KEY_ARG:
        args->ca_name = arg;
        break;
    case ARGP_KEY_END:
        if (!args->ca_name)
            argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    struct argp argp = { conty_options, conty_parse_opt, "NAME", doc};
    struct conty_args args = { .ca_timeout = -1, };

    if (argp_parse(&argp, argc, argv, 0, 0, &args) != 0)
        return 1;

    int container_fd, container_pid, ready, sigfd, i;
    sigset_t mask;
    struct conty_container *cc = NULL;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
        return 1;

    cc = conty_container_create(args.ca_name, args.ca_bundle);
    if (!cc)
        return 1;

    conty_container_set_status(cc, CONTY_CREATED);

    container_fd = conty_container_pollfd(cc);
    container_pid = conty_container_pid(cc);

    sigfd = signalfd(-1, &mask, SFD_CLOEXEC);
    if (sigfd < 0)
        goto kill_container;

    struct pollfd pollfds[2] = {
            { .fd = container_fd, .events = POLLIN, .revents = 0 },
            { .fd = sigfd,        .events = POLLIN, .revents = 0 }
    };

    if (conty_container_start(cc) != 0)
        goto close_sigfd;

    conty_container_set_status(cc, CONTY_RUNNING);

    for ( ;; ) {
        ready = poll(pollfds, 2, (int) args.ca_timeout);
        if (ready <= 0)
            break;

        if (pollfds[CONTY_CONTFD].revents & POLLIN) {
            waitpid(container_pid, NULL, 0);
            conty_container_set_status(cc, CONTY_STOPPED);
            close(sigfd);
            conty_container_delete(cc);
            return 0;
        }

        if (pollfds[CONTY_SIGFD].revents & POLLIN)
            goto close_sigfd;
    }

close_sigfd:
    close(sigfd);
kill_container:
    conty_container_kill(cc, SIGKILL);
    conty_container_set_status(cc, CONTY_STOPPED);
    conty_container_delete(cc);
    return 0;
}