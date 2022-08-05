#ifndef CONTY_CONF_H
#define CONTY_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "resource.h"

struct conty_conf {
    char *sandbox_id;
    char *rootfs;
    struct {
        char *veth_ip;
        char *bridge_ip;
    } net;
};

struct conty_conf *conty_conf_from_string(const char *string);
struct conty_conf *conty_conf_from_file(const char *path);

void conty_conf_free(struct conty_conf *conf);
CONTY_CREATE_CLEANUP_FUNC(struct conty_conf*, conty_conf_free);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_CONF_H
