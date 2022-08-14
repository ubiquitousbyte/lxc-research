#ifndef CONTY_SYNC_H
#define CONTY_SYNC_H

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "log.h"

enum {
    EVENT_ERROR,
    EVENT_RT_CREATE,
    EVENT_RT_CREATED,
    EVENT_CONT_CREATE,
    EVENT_CONT_CREATED,
    EVENT_CONT_START,
    EVENT_CONT_STARTED,
    EVENT_CONT_STOP,
    EVENT_CONT_STOPPED
};

static inline const char *conty_sync_event_str(int event)
{
    switch (event) {
        case EVENT_ERROR:
            return "EVENT_ERROR";
        case EVENT_RT_CREATE:
            return "EVENT_RUNTIME_CREATE";
        case EVENT_RT_CREATED:
            return "EVENT_RUNTIME_CREATED";
        case EVENT_CONT_CREATE:
            return "EVENT_CONTAINER_CREATE";
        case EVENT_CONT_CREATED:
            return "EVENT_CONTAINER_CREATED";
        case EVENT_CONT_START:
            return "EVENT_CONTAINER_START";
        case EVENT_CONT_STARTED:
            return "EVENT_CONTAINER_STARTED";
        case EVENT_CONT_STOP:
            return "EVENT_CONTAINER_STOP";
        case EVENT_CONT_STOPPED:
            return "EVENT_CONTAINER_STOPPED";
        default:
            return "EVENT_UNKNOWN";
    }
}

#define SYNC_FD_RT   0
#define SYNC_FD_CONT 1

/*
 * Interrupt-safe read operation on a file descriptor
 */
static inline ssize_t conty_sync_read(int fd, void *buf, size_t buf_len)
{
    ssize_t rx;
    do {
        rx = read(fd, buf, buf_len);
    } while (rx < 0 && errno == EINTR);
    return (rx < 0) ? -errno : rx;
}

/*
 * Interrupt-safe write operation on a file descriptor
 */
static inline ssize_t conty_sync_write(int fd, void *buf, size_t buf_len)
{
    ssize_t tx;
    do {
        tx = write(fd, buf, buf_len);
    } while (tx < 0 && errno == EINTR);
    return (tx < 0) ? -errno : tx;
}

/*
 * Initialise an inter-process communication pair
 */
static inline int conty_sync_init(int fds[2])
{
    int err;
    err = socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds);
    if (err != 0)
        return log_error_ret(-errno, "cannot create synchronisation mechanism");
    return err;
}

/*
 * Initialise the inter-process communication pair on the parent side
 * Must happen after the container has been forked
 */
static inline void conty_sync_init_runtime(int fds[2])
{
    close(fds[SYNC_FD_CONT]);
    fds[SYNC_FD_CONT] = -EBADF;
}

/*
 * Initialise the inter-process communication pair on the container side
 */
static inline void conty_sync_init_container(int fds[2])
{
    close(fds[SYNC_FD_RT]);
    fds[SYNC_FD_RT] = -EBADF;
}

/*
 * Wait for a particular event to arrive from the peer
 */
int conty_sync_wait(int fd, int event);

/*
 * Wake the peer with an event
 */
int conty_sync_wake(int fd, int event);

/*
 * Wait for a particular event from the runtime
 */
static inline int conty_sync_await_runtime(int fds[2], int event)
{
    LOG_TRACE("Child awaiting event %s", conty_sync_event_str(event));
    return conty_sync_wait(fds[SYNC_FD_CONT], event);
}

/*
 * Wait for a particular event from the container
 */
static inline int conty_sync_await_container(int fds[2], int event)
{
    LOG_TRACE("Parent awaiting event %s", conty_sync_event_str(event));
    return conty_sync_wait(fds[SYNC_FD_RT], event);
}

/*
 * Wake the runtime with an event
 */
static inline int conty_sync_wake_runtime(int fds[2], int event)
{
    LOG_TRACE("Child waking parent with event %s", conty_sync_event_str(event));
    return conty_sync_wake(fds[SYNC_FD_CONT], event);
}

/*
 * Wake the container with an event
 */
static inline int conty_sync_wake_container(int fds[2], int event)
{
    LOG_TRACE("Parent waking child with event %s", conty_sync_event_str(event));
    return conty_sync_wake(fds[SYNC_FD_RT], event);
}

/*
 * Synchronise state with the runtime
 */
static inline int conty_sync_runtime(int fds[2], int event)
{
    int err;
    if ((err = conty_sync_wake_runtime(fds, event)) != 0)
        return err;
    return conty_sync_await_runtime(fds, event + 1);
}

/*
 * Synchronise state with the container
 */
static inline int conty_sync_container(int fds[2], int event)
{
    int err;
    if ((err = conty_sync_wake_container(fds, event)) != 0)
        return err;
    return conty_sync_await_container(fds, event + 1);
}

#endif //CONTY_SYNC_H
