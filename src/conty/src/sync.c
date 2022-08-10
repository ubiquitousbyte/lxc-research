#include "sync.h"
#include "log.h"

#include <sys/socket.h>

int conty_sync_init(int syncfds[2])
{
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, syncfds) != 0)
        return LOG_ERROR_RET(-1, "cannot create ipc mechanism for synchronisation");
    return 0;
}

int conty_sync_wait(int syncfd, int expected)
{
    int event = -1;
    ssize_t rx;

    rx = conty_sync_read(syncfd, &event, sizeof(int));
    if (rx < 0)
        return LOG_ERROR_RET_ERRNO(-1, errno, "runtime<->container sync failure");

    if (rx != sizeof(int))
        return LOG_ERROR_RET(-1, "unexpected buffer in sync wait");

    if (event == SYNC_ERROR) {
        return LOG_ERROR_RET(-1, "peer emitted error sync %d (expected %d)",
                             event, expected);
    }

    if (event != expected)
        return LOG_ERROR_RET(-1, "sync mismatch %d (expected %d)", event, expected);

    return 0;
}

int conty_sync_wake_peer(int syncfd, int event)
{
    int e = event;
    ssize_t tx;
    tx = conty_sync_write(syncfd, &e, sizeof(int));
    if (tx < 0)
        return LOG_ERROR_RET_ERRNO(-1, errno, "runtime<->container sync failure");

    if ((size_t) tx != sizeof(int))
        return LOG_ERROR_RET(-1, "unexpected buffer in sync wait");

    return 0;
}

int conty_sync_transact(int syncfd, int event, int expected_event)
{
    if (conty_sync_wake_peer(syncfd, event) < 0)
        return -1;
    return conty_sync_wait(syncfd, expected_event);
}