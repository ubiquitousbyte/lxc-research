#ifndef CONTY_RUNTIME_H
#define CONTY_RUNTIME_H

#include <conty/conty.h>
#include <signal.h>
#include <sys/un.h>

#include "hash.h"

#define CONTY_RT_BUFSIZE 4096

enum {
    CONTY_RT_CREATE,
    CONTY_RT_START,
    CONTY_RT_KILL,
    CONTY_RT_DELETE
};

struct conty_rt_server_buf {
    char   sb_rx[CONTY_RT_BUFSIZE];
    char   sb_tx[CONTY_RT_BUFSIZE];
    int    sb_op;
    char  *sb_container_id;
    char  *sb_params[3];
};

static inline int conty_request_op_from_str(const char *str)
{
    if (!strncmp(str, "create", sizeof("create") - 1))
        return CONTY_RT_CREATE;

    if (!strncmp(str, "start", sizeof("start") - 1))
        return CONTY_RT_START;

    if (!strncmp(str, "kill", sizeof("kill") - 1))
        return CONTY_RT_KILL;

    if (!strncmp(str, "delete", sizeof("delete") - 1))
        return CONTY_RT_DELETE;

    return -EINVAL;
}

static inline int conty_signal(const char *sig)
{
    if (!strncmp(sig, "SIGKILL", sizeof("SIGKILL") - 1))
        return SIGKILL;

    if (!strncmp(sig, "SIGTERM", sizeof("SIGTERM") - 1))
        return SIGTERM;

    if (!strncmp(sig, "SIGABRT", sizeof("SIGABRT") - 1))
        return SIGABRT;

    return -EINVAL;
}

struct conty_rt_server {
    int                rts_fd;
    struct sockaddr_un rts_addr;
};

int conty_rt_server_init(struct conty_rt_server *server, const char *path);
int conty_rt_server_listen(const struct conty_rt_server *server);
int conty_rt_server_accept_conn(const struct conty_rt_server *server);
void conty_rt_server_close(struct conty_rt_server *server);

typedef int conty_rt_loop_t;

conty_rt_loop_t conty_rt_loop_open(void);
int conty_rt_loop_add_fd(conty_rt_loop_t loop, int fd, uint32_t events, void *data);
int conty_rt_loop_del_fd(conty_rt_loop_t loop, int fd);
void conty_rt_loop_close(conty_rt_loop_t loop);

/*
 * Hashable container
 */
struct conty_rt_hc {
    char                   *hc_id;
    struct conty_container *hc_cc;
    UT_hash_handle          hh;
};

struct conty_rt;

typedef int (*conty_rt_request_handler)(struct conty_rt *rt,
                                        struct conty_rt_server_buf *req);

struct conty_rt {
    struct conty_rt_server    rt_server;
    conty_rt_loop_t           rt_loop;
    struct conty_rt_hc       *rt_containers;
    conty_rt_request_handler  rt_handlers[CONTY_RT_DELETE + 1];
};

int conty_rt_init(struct conty_rt *rt, const char *server_path);

int conty_rt_register_handler(struct conty_rt *rt, int request,
                              conty_rt_request_handler h);

int conty_rt_run(struct conty_rt *rt);

void conty_rt_free(struct conty_rt *rt);

#endif //CONTY_RUNTIME_H
