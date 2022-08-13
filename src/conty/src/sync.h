#ifndef CONTY_SYNC_H
#define CONTY_SYNC_H

#include <stddef.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>

#include "log.h"

enum {
    /* Child or parent failed with an error condition */
    SYNC_ERROR,
    /* Request the creation of a runtime environment from the runtime */
    SYNC_RUNTIME_CREATE,
    /* Acknowledge the creation of a runtime environment by the runtime */
    SYNC_RUNTIME_CREATED,
    /* Request the creation of a container environment from the container */
    SYNC_CONTAINER_CREATE,
    /* Acknowledge the creation of a container environment by the container */
    SYNC_CONTAINER_CREATED,
    /* Request execution of the user-defined application by the container */
    SYNC_CONTAINER_START,
    /* Acknowledge that the user-defined application was executed */
    SYNC_CONTAINER_STARTED,
};

#define CONTY_SYNC_MAX (SYNC_CONTAINER_STARTED + 1)
#define CONTY_SYNC_CFD 0
#define CONTY_SYNC_RTFD  1

static const char *conty_sync_str[CONTY_SYNC_MAX] = {
        [SYNC_ERROR]             = "SYNC_ERROR",
        [SYNC_RUNTIME_CREATE]    = "SYNC_RUNTIME_CREATE",
        [SYNC_RUNTIME_CREATED]   = "SYNC_RUNTIME_CREATED",
        [SYNC_CONTAINER_CREATE]  = "SYNC_CONTAINER_CREATE",
        [SYNC_CONTAINER_CREATED] = "SYNC_CONTAINER_CREATED",
        [SYNC_CONTAINER_START]   = "SYNC_CONTAINER_START",
        [SYNC_CONTAINER_STARTED] = "SYNC_CONTAINER_STARTED"
};

static inline ssize_t conty_sync_read(int syncfd, void *buf, size_t buf_len)
{
    ssize_t rx;
    do {
        rx = read(syncfd, buf, buf_len);
    } while (rx < 0 && errno == EINTR);
   return rx;
}

static inline ssize_t conty_sync_write(int syncfd, void *buf, size_t buf_len)
{
    ssize_t tx;
    do {
        tx = write(syncfd, buf, buf_len);
    } while (tx < 0 && errno == EINTR);
    return tx;
}

int conty_sync_init(int syncfds[2]);

int conty_sync_wait(int syncfd, int expected_event);
int conty_sync_wake_peer(int syncfd, int event);
int conty_sync_transact(int syncfd, int event, int expected_event);

static inline void conty_sync_parent_init(int syncfds[2])
{
    close(syncfds[CONTY_SYNC_CFD]);
    syncfds[CONTY_SYNC_CFD] = -EBADF;
}

static inline void conty_sync_child_init(int syncfds[2])
{
    close(syncfds[CONTY_SYNC_RTFD]);
    syncfds[CONTY_SYNC_RTFD] = -EBADF;
}

static inline int conty_sync_parent_wait(int syncfds[2], int expected_event)
{
    LOG_TRACE("Parent waiting for event %s", conty_sync_str[expected_event]);
    return conty_sync_wait(syncfds[CONTY_SYNC_RTFD], expected_event);
}

static inline int conty_sync_child_wait(int syncfds[2], int expected_event)
{
    LOG_TRACE("Child waiting for event %s", conty_sync_str[expected_event]);
    return conty_sync_wait(syncfds[CONTY_SYNC_CFD], expected_event);
}

static inline int conty_sync_parent_wake(int syncfds[2], int expected_event)
{
    LOG_TRACE("Parent sending event %s", conty_sync_str[expected_event]);
    return conty_sync_wake_peer(syncfds[CONTY_SYNC_RTFD], expected_event);
}

static inline int conty_sync_child_wake(int syncfds[2], int expected_event)
{
    LOG_TRACE("Child sending event %s", conty_sync_str[expected_event]);
    return conty_sync_wake_peer(syncfds[CONTY_SYNC_CFD], expected_event);
}

static inline int conty_sync_parent_tx(int syncfds[2], int event, int expected_event)
{
    LOG_TRACE("Parent sending event %s and waiting for %s",
              conty_sync_str[event], conty_sync_str[expected_event]);
    return conty_sync_transact(syncfds[CONTY_SYNC_RTFD], event, expected_event);
}

static inline int conty_sync_child_tx(int syncfds[2], int event, int expected_event)
{
    LOG_TRACE("Child sending event %s and waiting for %s",
              conty_sync_str[event], conty_sync_str[expected_event]);
    return conty_sync_transact(syncfds[CONTY_SYNC_CFD], event, expected_event);
}

#endif //CONTY_SYNC_H
