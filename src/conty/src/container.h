#ifndef CONTY_CONTAINER_H
#define CONTY_CONTAINER_H

#ifdef __cplusplus
extern "C" {
#endif

struct oci_conf;

struct conty_container;

struct conty_container *conty_container_create(const char *cc_id,
                                               const struct oci_conf *conf);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_CONTAINER_H
