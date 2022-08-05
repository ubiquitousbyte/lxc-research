#include "network.h"

#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/link/veth.h>

#include "resource.h"
#include "log.h"

CONTY_CREATE_CLEANUP_FUNC(struct rtnl_link *, rtnl_link_put);

CONTY_CREATE_CLEANUP_FUNC(struct nl_sock *, nl_socket_free);

int conty_network_veth_create(const char *hname, const char *sbname, pid_t nspid)
{
    int err;
    CONTY_INVOKE_CLEANER(nl_socket_free) struct nl_sock *sock = NULL;

    /*
     * Open up a netlink socket to the routing subsystem in the kernel
     */
    sock = nl_socket_alloc();
    if (!sock)
        return -LOG_FATAL_RET(-ENOMEM, "conty_network: out of memory");

    /*
     * Create a virtual ethernet device pair and move the
     * peer end of the device into the network namespace of the
     * process defined by nspid
     */
    err = rtnl_link_veth_add(sock, hname, sbname, nspid);
    if (err != 0) {
        return -LOG_ERROR_RET(err, "conty_network: cannot create veth pair %s:%s",
                              hname, sbname);
    }

    /*
     * TODO: Now that we've created the veth pair, we need to configure an
     * TODO: L2 virtual switch a.k.a a bridge that will forward packets
     * TODO: between the sandbox, the host and the internet/other sandboxes on the system.
     */

    return 0;
}