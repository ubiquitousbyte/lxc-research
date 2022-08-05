#ifndef CONTY_NETWORK_H
#define CONTY_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <unistd.h>
#include <net/if.h>

/*
 * Request object for creating a virtual ethernet device pair
 * One end of the pair will be attached to the sandbox and the other
 * will be attached to the host.
 *
 * Traffic on one end will be forwarded to the other
*/
struct conty_network_veth_request {
    /*
     * Name of veth device on the host
     */
    char hname[IFNAMSIZ];
    /*
     * Name of veth device in the sandbox
     */
    char sbname[IFNAMSIZ];
    /*
     * Process identifier of a process residing in the network namespace
     * that should own one end of the virtual ethernet device
     */
    pid_t nspid;
};

int conty_network_veth_create(const char *hname, const char *sbname, pid_t nspid);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_NETWORK_H
