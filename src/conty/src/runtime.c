#include <conty/conty.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/wait.h>

#include "hash.h"
#include "runtime.h"
#include "log.h"

#define MAX_EVENTS 256

#define strnprintf(buf, size, ...)                                            \
    ({                                                                        \
        int __internal__ret;                                                  \
        __internal__ret = snprintf(buf, size, ##__VA_ARGS__);                 \
        if (__internal__ret < 0 || (size_t) __internal__ret >= (size_t) size) \
            __internal__ret = -EIO;                                           \
        __internal__ret;                                                      \
    })

static volatile sig_atomic_t exiting = 0;

static void sig_int(int signo)
{
    exiting = 1;
}

struct conty_rt_event {
    int                     ev_fd;
    struct conty_container *ev_cc;
};

static struct conty_rt_event *conty_rt_event_create(void)
{
    struct conty_rt_event *event;

    event = malloc(sizeof(struct conty_rt_event));
    if (!event)
        return log_error_ret(NULL, "out of memory");

    event->ev_fd = -EBADF;
    event->ev_cc = NULL;

    return event;
}

static void conty_rt_event_free(struct conty_rt_event *event)
{
    if (event) {
        if (event->ev_fd >= 0)
            close(event->ev_fd);
        free(event);
        event = NULL;
    }
}

static int conty_rt_create_container(struct conty_rt *rt,
                                     struct conty_rt_server_buf *req);
static int conty_rt_start_container(struct conty_rt *rt,
                                    struct conty_rt_server_buf *req);
static int conty_rt_kill_container(struct conty_rt *rt,
                                   struct conty_rt_server_buf *req);
static int conty_rt_delete_container(struct conty_rt *rt,
                                     struct conty_rt_server_buf *req);

static conty_rt_request_handler conty_rt_default_handlers[CONTY_RT_DELETE + 1] = {
        [CONTY_RT_CREATE] = conty_rt_create_container,
        [CONTY_RT_START]  = conty_rt_start_container,
        [CONTY_RT_KILL]   = conty_rt_kill_container,
        [CONTY_RT_DELETE] = conty_rt_delete_container
};

int conty_rt_server_init(struct conty_rt_server *server, const char *path)
{
    int unixfd, err;
    struct sockaddr_un *addr = &server->rts_addr;

    memset(addr, 0, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, path, sizeof(addr->sun_path) - 1);

    unixfd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (unixfd < 0)
        return log_error_ret(-errno, "cannot create socket server at %s", path);

    if (bind(unixfd, (struct sockaddr *) addr, sizeof(*addr)) != 0) {
        err = -errno;
        close(unixfd);
        return log_error_ret(err, "cannot bind socket server to %s", path);
    }

    server->rts_fd = unixfd;

    return 0;
}

int conty_rt_server_listen(const struct conty_rt_server *server)
{
    if (listen(server->rts_fd, 2) != 0)
        return -errno;
    return 0;
}

int conty_rt_server_accept_conn(const struct conty_rt_server *server)
{
    int fd;

    if ((fd = accept(server->rts_fd, NULL, NULL)) < 0)
        return -errno;

    return fd;
}

void conty_rt_server_close(struct conty_rt_server *server)
{
    if (server) {
        close(server->rts_fd);
        unlink(server->rts_addr.sun_path);
    }
}

conty_rt_loop_t conty_rt_loop_open(void)
{
    int res = epoll_create1(EPOLL_CLOEXEC);
    return (res < 0) ? -errno : res;
}

int conty_rt_loop_add_fd(conty_rt_loop_t loop, int fd, uint32_t events, void *data)
{
    struct epoll_event event;

    event.events = events;
    if (data)
        event.data.ptr = data;
    else
        event.data.fd = fd;

    return (epoll_ctl(loop, EPOLL_CTL_ADD, fd, &event) < 0) ? -errno : 0;
}

int conty_rt_loop_del_fd(conty_rt_loop_t loop, int fd)
{
    return (epoll_ctl(loop, EPOLL_CTL_DEL, fd, NULL) < 0) ? -errno : 0;
}

void conty_rt_loop_close(conty_rt_loop_t loop)
{
    if (loop >= 0)
        close(loop);
}

int conty_rt_init(struct conty_rt *rt, const char *server_path)
{
    int err;

    if ((err = conty_rt_server_init(&rt->rt_server, server_path)) != 0)
        return err;

    err = conty_rt_loop_open();
    if (err < 0)
        goto cleanup;

    rt->rt_loop       = err;
    rt->rt_containers = NULL;

    for (int i = CONTY_RT_CREATE; i <= CONTY_RT_DELETE; i++)
        rt->rt_handlers[i] = conty_rt_default_handlers[i];

    return 0;

cleanup:
    conty_rt_server_close(&rt->rt_server);
    return err;
}

int conty_rt_register_handler(struct conty_rt *rt, int request,
                              conty_rt_request_handler h)
{
    if (request < CONTY_RT_CREATE || request > CONTY_RT_DELETE)
        return -EINVAL;

    if (!h)
        return -EINVAL;

    rt->rt_handlers[request] = h;

    return 0;
}

static int conty_rt_request_read(struct conty_rt_server_buf *req, int cfd)
{
    ssize_t rx;
    char *tok, *save_ptr;
    int i;

    memset(req, 0, sizeof(struct conty_rt_server_buf));

    do {
        rx = read(cfd, req->sb_rx, sizeof(req->sb_rx));
    } while (rx < 0 && errno == EINTR);

    if (rx < 0)
        return -errno;

    if (rx == 0)
        return -ENODATA;

    tok = strtok_r(req->sb_rx, " ", &save_ptr);
    if (!tok || (req->sb_op = conty_request_op_from_str(tok)) < CONTY_RT_CREATE)
        return -EOPNOTSUPP;

    LOG_INFO("Received request %s", tok);

    req->sb_container_id = strtok_r(NULL, " ", &save_ptr);
    if (!req->sb_container_id)
        return -ESRCH;

    for (i = 0; i < 2; i++) {
        tok = strtok_r(NULL, " ", &save_ptr);
        if (!tok)
            break;
        req->sb_params[i] = tok;
    }

    req->sb_params[i] = NULL;

    return 0;
}

static int conty_rt_handle_request(struct conty_rt *rt,
                                   struct conty_rt_server_buf *buf,
                                   int cfd)
{
    ssize_t tx;
    const char *msg;
    int err;

    err = conty_rt_request_read(buf, cfd);
    if (err == 0)
        msg = "ok";
    else if (err == -EOPNOTSUPP)
        msg = "invalid command";
    else if (err == -ESRCH)
        msg = "container not found";
    else
        return err;

    strnprintf(buf->sb_tx, sizeof(buf->sb_tx), "%s", msg);

    if (err != 0)
        goto write_response;

    err = rt->rt_handlers[buf->sb_op](rt, buf);
    if (err != 0)
        strnprintf(buf->sb_tx, sizeof(buf->sb_tx), "%s", strerror(-err));

write_response:
    do {
        tx = write(cfd, buf->sb_tx, sizeof(buf->sb_tx));
    } while (tx < 0 && errno == EINTR);
    if (tx == 0)
        return -ENODATA;
    if (tx < 0)
        return -errno;

    return 0;
}

static int conty_rt_reap_container(struct conty_container *cc)
{
    pid_t reaped_pid;
    pid_t pid = conty_container_pid(cc);

    do {
        reaped_pid = waitpid(pid, NULL, 0);
    } while (reaped_pid < 0 && errno == EINTR);
    if (reaped_pid != pid)
        return -errno;

    conty_container_set_status(cc, CONTY_STOPPED);

    return 0;
}

int conty_rt_run(struct conty_rt *rt)
{
    int err, nfds, i;
    struct conty_rt_event *cur = NULL, se, *new = NULL;
    struct epoll_event events[MAX_EVENTS];
    struct conty_rt_server_buf buf;
    struct conty_rt_hc *hc;

    se.ev_fd = rt->rt_server.rts_fd;
    se.ev_cc = NULL;

    if ((err = conty_rt_server_listen(&rt->rt_server)) != 0)
        goto out;

    err = conty_rt_loop_add_fd(rt->rt_loop, se.ev_fd, EPOLLIN, &se);
    if (err < 0)
        goto out;

    while (!exiting) {
        nfds = epoll_wait(rt->rt_loop, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR)
                continue;
            goto out;
        }

        for (i = 0; i < nfds; i++) {
            if (events[i].events & EPOLLIN) {
                cur = (struct conty_rt_event *) events[i].data.ptr;

                if (cur->ev_cc) {
                    LOG_INFO("Reaping container %s", conty_container_id(cur->ev_cc));

                    if ((err = conty_rt_reap_container(cur->ev_cc)) != 0)
                        return err;

                    if ((err = conty_rt_loop_del_fd(rt->rt_loop, cur->ev_fd)) != 0)
                        return -1;

                    conty_rt_event_free(cur);
                    continue;
                }

                if (cur->ev_fd == rt->rt_server.rts_fd) {
                    LOG_INFO("Accepting connection");

                    new = conty_rt_event_create();
                    if (!new) {
                        goto out;
                    }

                    new->ev_fd = conty_rt_server_accept_conn(&rt->rt_server);
                    if (new->ev_fd < 0)
                        goto out;

                    err = conty_rt_loop_add_fd(rt->rt_loop, new->ev_fd,
                                               EPOLLIN | EPOLLHUP | EPOLLERR,
                                               new);
                    if (err < 0)
                        goto out;

                    continue;
                }

                err = conty_rt_handle_request(rt, &buf, cur->ev_fd);
                if (err == -ENODATA) {
                    if (conty_rt_loop_del_fd(rt->rt_loop, cur->ev_fd) != 0)
                        goto out;
                    conty_rt_event_free(cur);
                    continue;
                } else if (err != 0)
                    goto out;

                if (buf.sb_op == CONTY_RT_CREATE) {
                    new = conty_rt_event_create();
                    if (!new)
                        goto out;

                    HASH_FIND_STR(rt->rt_containers, buf.sb_container_id, hc);
                    if (!hc)
                        goto out;

                    new->ev_fd = conty_container_pollfd(hc->hc_cc);
                    new->ev_cc = hc->hc_cc;

                    err = conty_rt_loop_add_fd(rt->rt_loop, new->ev_fd,
                                               EPOLLIN | EPOLLHUP | EPOLLERR,
                                               new);
                    if (err < 0)
                        goto out;
                }
            }
        }
    }
out:
    conty_rt_event_free(new);
    return err;
}

void conty_rt_free(struct conty_rt *rt)
{
    if (rt) {
        conty_rt_server_close(&rt->rt_server);
        conty_rt_loop_close(rt->rt_loop);
    }
}

static int conty_rt_create_container(struct conty_rt *rt,
                                     struct conty_rt_server_buf *req)
{
    const char *bundle_path = req->sb_params[0];
    struct conty_container *cc = NULL;
    struct conty_rt_hc *hc = NULL;

    if (!bundle_path)
        return -EINVAL;

    if (access(bundle_path, R_OK) != 0)
        return -errno;

    HASH_FIND_STR(rt->rt_containers, req->sb_container_id, hc);
    if (hc)
        return -EEXIST;

    hc = calloc(1, sizeof(struct conty_rt_hc));
    if (!hc)
        return -ENOMEM;

    cc = conty_container_create(req->sb_container_id, bundle_path);
    if (!cc) {
        free(hc);
        return -ECHILD;
    }

    conty_container_set_status(cc, CONTY_CREATED);

    hc->hc_cc = cc;
    hc->hc_id = strdup(req->sb_container_id);

    HASH_ADD_KEYPTR(hh, rt->rt_containers, hc->hc_id, strlen(hc->hc_id), hc);

    return 0;
}

static int conty_rt_start_container(struct conty_rt *rt,
                                    struct conty_rt_server_buf *req)
{
    int err;
    const char *container_id = req->sb_container_id;
    struct conty_rt_hc *hc = NULL;

    HASH_FIND_STR(rt->rt_containers, container_id, hc);
    if (!hc)
        return -ENOENT;

    if (conty_container_status(hc->hc_cc) != CONTY_CREATED)
        return -EINVAL;

    if ((err = conty_container_start(hc->hc_cc)) != 0)
        return err;

    conty_container_set_status(hc->hc_cc, CONTY_RUNNING);

    return err;
}

static int conty_rt_kill_container(struct conty_rt *rt,
                                   struct conty_rt_server_buf *req)
{
    int sig;
    const char *signal = req->sb_params[0];
    const char *container_id = req->sb_container_id;
    struct conty_rt_hc *hc = NULL;

    if (!signal)
        return -EINVAL;

    if ((sig = conty_signal(signal)) < 0)
        return -EINVAL;

    HASH_FIND_STR(rt->rt_containers, container_id, hc);
    if (!hc)
        return -ENOENT;

    if (conty_container_status(hc->hc_cc) != CONTY_RUNNING)
        return -EINVAL;

    return conty_container_kill(hc->hc_cc, sig);
}

static int conty_rt_delete_container(struct conty_rt *rt,
                                     struct conty_rt_server_buf *req)
{
    int err;
    const char *container_id = req->sb_container_id;
    struct conty_rt_hc *hc = NULL;

    HASH_FIND_STR(rt->rt_containers, container_id, hc);
    if (!hc)
        return -ENOENT;

    if (conty_container_status(hc->hc_cc) != CONTY_STOPPED)
        return -EINVAL;

    HASH_DEL(rt->rt_containers, hc);

    err = conty_container_delete(hc->hc_cc);

    free(hc->hc_id);
    free(hc);

    return err;
}

int main(int argc, char *argv[])
{
    int err;
    if (argc != 2)
        return EXIT_FAILURE;

    if (signal(SIGINT, sig_int) == SIG_ERR)
        return log_error_ret(EXIT_FAILURE, "cannot set signal handler");

    const char *socket_path = argv[1];
    struct conty_rt rt;

    if (conty_rt_init(&rt, socket_path) != 0)
        return log_error_ret(EXIT_FAILURE, "cannot initialise runtime");

    err = conty_rt_run(&rt);

    conty_rt_free(&rt);

    return (err != 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
