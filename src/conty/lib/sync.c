#include "sync.h"

int conty_sync_wait(int fd, int event)
{
    int tmp = -1;
    ssize_t rx;

    rx = conty_sync_read(fd, &tmp, sizeof(int));
    if (rx < 0)
        return log_error_ret(rx, "event could not be awaited");

    if (rx != sizeof(int) || tmp != event)
        return log_error_ret(rx, "wait returned unexpected event");

    return 0;
}

int conty_sync_wake(int fd, int event)
{
    ssize_t tx;

    tx = conty_sync_write(fd, &event, sizeof(event));
    if (tx < 0)
        return log_error_ret(tx, "could not wake peer with event");

    if (tx == 0)
        return log_error_ret(-ECONNABORTED, "cannot contact peer");

    if (tx != sizeof(int))
        return log_error_ret(-EIO, "woke child with unexpected event");

    return 0;
}