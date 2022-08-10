#ifndef CONTY_CONTAINER_H
#define CONTY_CONTAINER_H
#include <conty/conty.h>

#include <unistd.h>

#include "resource.h"
#include "namespace.h"
#include "mount.h"
#include "user.h"

struct conty_container {
    /* Management data */
    const char            *cc_id;
    pid_t                  cc_pid;
    int                    cc_pollfd;
    int                    cc_syncfds[2];
    oci_container_status_t cc_oci_status;


    /* Namespace state bookkeeping */
    struct {
        unsigned long cc_ns_new;
        int           cc_ns_fds[CONTY_NS_SIZE];
        char          cc_ns_has_fds;
    };

    /* Mount namespace configuration */
    struct {
        struct conty_rootfs cc_mnt_root;
    };

    struct oci_conf *cc_oci_conf;
};

struct conty_container *conty_container_new(const char *cc_id, const char *path);
void conty_container_get_state(const struct conty_container *cc,
                               struct oci_process_state *state);

void conty_container_free(struct conty_container *cc);

CONTY_CREATE_CLEANUP_FUNC(struct conty_container *, conty_container_free);
CONTY_CREATE_CLEANUP_FUNC(struct oci_conf *, oci_conf_free);

#endif //CONTY_CONTAINER_H
