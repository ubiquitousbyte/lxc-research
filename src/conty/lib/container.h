#ifndef CONTY_CONTAINER_H
#define CONTY_CONTAINER_H

#include <conty/conty.h>

#include <unistd.h>

#include "namespace.h"
#include "oci.h"

struct conty_container {
    /*
     * Container identifier
     */
    const char               *cc_id;
    conty_container_status_t  cc_status;
    /*
     * Container process identifier
     */
    pid_t cc_pid;
    /*
     * Pollable file descriptor for the container process
     * Right now, we don't support daemonless containers, so the
     * caller must always use this or the pid to eventually reap the child
     */
    int cc_pollfd;
    /*
     * File descriptors for inter-process synchronisation
     * between the runtime and the container
     */
    int cc_syncfds[2];
    struct {
        /*
         * New namespaces to set up
         */
        unsigned long cc_ns_new;
        /*
         * File descriptors to already existing namespaces
         */
        int cc_ns_fds[CONTY_NS_LEN];
        /*
         * Bookkeeping flag that tells us if cc_ns_fds has
         * an open file descriptor inside
         */
        char cc_ns_has_fds;
    };
    /*
     * OCI configuration
     */
    struct oci_conf *cc_conf;
};

static inline int conty_container_pollfd(const struct conty_container *cc)
{
    return cc->cc_pollfd;
}

static inline void conty_container_set_status(struct conty_container *cc,
                                              conty_container_status_t status)
{
    cc->cc_status = status;
}

static inline conty_container_status_t conty_container_status(const struct conty_container *container)
{
    return container->cc_status;
}

static inline const char *conty_container_status_str(const struct conty_container *container)
{
    static const char *status_str[CONTY_STATUS_MAX + 1] = {
            [CONTY_CREATING] = "creating",
            [CONTY_CREATED]  = "created",
            [CONTY_RUNNING]  = "running",
            [CONTY_STOPPED]  = "stopped"
    };
    return status_str[container->cc_status];
}

int conty_container_init(struct conty_container *cc, const char *id, const char *bundle);
int conty_container_spawn(struct conty_container *cc);

void conty_container_free(struct conty_container *container);

CREATE_CLEANER(struct conty_container *, conty_container_free);
#define CONTAINER_MEM_RESOURCE MAKE_RESOURCE(conty_container_free)

#endif //CONTY_CONTAINER_H
