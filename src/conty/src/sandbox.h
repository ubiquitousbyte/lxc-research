#ifndef CONTY_SANDBOX_H
#define CONTY_SANDBOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "namespace.h"
#include "hook.h"
#include "queue.h"

struct conty_sandbox {
    /*
     * Set of namespaces to move the process into
     */
    int                                       ns_new;
    /*
     * Set of already existing namespaces that the process will join
     */
    TAILQ_HEAD(conty_sandbox_setns, conty_ns) ns_existing;
    /*
     * Event hooks to be triggered at different points during
     * the sandboxing process
     */
    struct conty_event_hooks                  hooks;
    /*
     * Identifier mappings between the parent user namespace and the
     * sandbox user namespace.
     */
    struct {
        struct conty_ns_id_map uid;
        struct conty_ns_id_map gid;
    } *id_map;


};

int conty_sandbox_create(struct conty_sandbox *sandbox);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_SANDBOX_H
