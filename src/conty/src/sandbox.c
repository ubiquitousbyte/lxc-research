#include "sandbox.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>

int conty_sandbox_create(struct conty_sandbox *sandbox)
{
    int err;
    int ipc_fds[2];

    /*
     * The primary method of communication between the parent and the child
     * will be a connected pair of sockets transmitting sequenced datagrams
     */
    err = socketpair(AF_UNIX, SOCK_SEQPACKET, 0, ipc_fds);
    if (err != 0)
        return -errno;


}