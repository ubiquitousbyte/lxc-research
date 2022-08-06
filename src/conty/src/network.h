#ifndef CONTY_NETWORK_H
#define CONTY_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

#include <net/if.h>

#include "resource.h"

struct conty_network;


struct conty_network *conty_network_create(const char bridge[IFNAMSIZ],
                                           const char veth0[IFNAMSIZ],
                                           const char veth1[IFNAMSIZ],
                                           pid_t nspid);

void conty_network_destroy(struct conty_network *net);
CONTY_CREATE_CLEANUP_FUNC(struct conty_network *, conty_network_destroy);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_NETWORK_H
