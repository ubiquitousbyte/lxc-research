#include "network.h"

#include <net/if.h>

#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/link/veth.h>
#include <netlink/route/link/bridge.h>

#include "resource.h"
#include "log.h"

struct conty_network {
    /*
     * Netlink socket used for communicating with the kernel
     * and configuring network interfaces, ip addresses, routes etc
     */
    struct nl_sock *sock;
    /*
     * L2 packet forwarder
     */
    struct rtnl_link *bridge;
    /*
     * Virtual ethernet cable that will be attached to the bridge
     * in order to enable communication across network namespace boundaries
     */
    struct rtnl_link *veth;
};

struct conty_network *conty_network_create(const char bridge[IFNAMSIZ],
                                           const char veth0[IFNAMSIZ],
                                           const char veth1[IFNAMSIZ],
                                           pid_t nspid)
{
    int err;
    CONTY_INVOKE_CLEANER(conty_network_destroy) struct conty_network *net = NULL;
    struct rtnl_link *veth_peer;

    net = calloc(1, sizeof(struct conty_network));
    if (!net)
        return LOG_FATAL_RET(NULL, "conty_network: out of memory");

    /*
     * Open up a netlink socket to the routing subsystem in the kernel
     */
    net->sock = nl_socket_alloc();
    if (!net->sock)
        return LOG_FATAL_RET(NULL, "conty_network: out of memory");

    err = nl_connect(net->sock, NETLINK_ROUTE);
    if (err != 0)
        return LOG_ERROR_RET(NULL, "conty_network: cannot connect to rtnetlink");

    /*
     * Create a virtual ethernet device pair
     */
    net->veth = rtnl_link_veth_alloc();
    if (!net->veth)
        return LOG_FATAL_RET(NULL, "conty_network: out of memory");

    veth_peer = rtnl_link_veth_get_peer(net->veth);
    rtnl_link_set_name(net->veth, veth0);
    rtnl_link_set_name(veth_peer, veth1);
    /*
     * Make sure to move the peer end of the device into the network
     * namespace of the process identified by nspid
     */
    rtnl_link_set_ns_pid(veth_peer, nspid);
    /*
     * Issue the creation request to the kernel
     */
    err = rtnl_link_add(net->sock, net->veth, NLM_F_CREATE);
    if (err != 0) {
        return LOG_ERROR_RET(NULL, "conty_network: cannot create veth %s<->%s: %s",
                             veth0, veth1, nl_geterror(err));
    }

    rtnl_link_set_ifindex(net->veth, if_nametoindex(veth0));

    /*
     * Now we'll create an L2 virtual switch on the host that will
     * forward packets between our sandbox and other peers attached to the switch
     */
    net->bridge = rtnl_link_bridge_alloc();
    if (!net->bridge)
        return LOG_FATAL_RET(NULL, "conty_network: out of memory");

    rtnl_link_set_name(net->bridge, bridge);
    err = rtnl_link_add(net->sock, net->bridge, NLM_F_CREATE);
    if (err != 0) {
        return LOG_ERROR_RET(NULL, "conty_network: could not create bridge %s: %s",
                             bridge, nl_geterror(err));
    }
    rtnl_link_set_ifindex(net->bridge, if_nametoindex(bridge));

    /*
     * Attach host portion of the virtual ethernet device to the bridge
     */
    err = rtnl_link_enslave(net->sock, net->bridge, net->veth);
    if (err != 0) {
        return LOG_ERROR_RET(NULL, "conty_network: %s could not enslave %s: %s",
                             bridge, veth0, nl_geterror(err));
    }

    return CONTY_MOVE_PTR(net);
}

void conty_network_destroy(struct conty_network *net)
{
    if (net) {
        if (net->sock) {
            if (net->veth)
                rtnl_link_delete(net->sock, net->veth);
            if (net->bridge)
                rtnl_link_delete(net->sock, net->bridge);
            nl_socket_free(net->sock);
            net->sock = NULL;
        }

        if (net->veth) {
            rtnl_link_veth_release(net->veth);
            net->veth = NULL;
        }

        if (net->bridge) {
            rtnl_link_put(net->bridge);
            net->bridge = NULL;
        }
    }
}

