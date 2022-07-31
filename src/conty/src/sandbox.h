#ifndef CONTY_SANDBOX_H
#define CONTY_SANDBOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "namespace.h"
#include "hook.h"

typedef enum conty_sandbox_ns_idx_t {
    CONTY_SANDBOX_NS_USER = 0,
    CONTY_SANDBOX_NS_PID  = 1,
    CONTY_SANDBOX_NS_MNT  = 2,
    CONTY_SANDBOX_NS_NET  = 3,
    CONTY_SANDBOX_NS_UTS  = 4,
    CONTY_SANDBOX_NS_IPC  = 5,
    CONTY_SANDBOX_NS_MAX  = 6,
} conty_sandbox_ns_idx_t;

struct conty_sandbox {
    struct {
        /*
         * Subset of the entire set that refers to namespaces that need
         * to be created
         */
        unsigned int clone_flags;
        /*
         * Subset of the entire set that refers to namespaces that already
         * exist and need to be joined
         */
        struct conty_ns *namespaces[CONTY_SANDBOX_NS_MAX];
    } ns;
    /*
     * Event hooks to be triggered at different points during
     * the sandboxing procedure
     */
    struct conty_event_hooks hooks;
    /*
     * Identifier mappings between the parent user namespace and the
     * sandbox user namespace.
     * Only valid when ns->all has CLONE_NEWUSER
     */
    struct {
        struct conty_ns_id_map users;
        struct conty_ns_id_map groups;
    } *id_map;
    /*
     * Socket file descriptors used for inter-process communication
     * between the sandbox and the runtime
     */
    int ipc_fds[2];
    /*
     * Process identifier of sandbox
     */
    pid_t pid;
    /*
     * Pollable process file descriptor for the sandbox
     */
    int pidfd;
};

int conty_sandbox_create(struct conty_sandbox *sandbox);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_SANDBOX_H
