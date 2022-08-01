#ifndef CONTY_SANDBOX_H
#define CONTY_SANDBOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "namespace.h"
#include "hook.h"
#include "user.h"
#include "queue.h"

struct conty_sandbox {
    struct {
        /*
         * Namespaces to create
         */
        unsigned long new;
        /*
         * Namespaces to join
         */
        unsigned long old;
        /*
         * File descriptors of joined namespaces
         * Initially, this contains the file descriptors
         * of namespaces defined in old
         */
        int           fds[CONTY_NS_SIZE];
    };

    /*
     * Event hooks to trigger at different points of the sandboxing procedure
     */
    struct {
        TAILQ_HEAD(__conty_rt_hooks, conty_hook)      on_rt_create;
        TAILQ_HEAD(__conty_created_hooks, conty_hook) on_sb_created;
        TAILQ_HEAD(__conty_start_hooks, conty_hook)   on_sb_start;
        TAILQ_HEAD(__conty_started_hooks, conty_hook) on_sb_started;
        TAILQ_HEAD(__conty_stopped_hooks, conty_hook) on_sb_stopped;
    } hooks;

    /*
     * Identifier mappings between the parent user namespace and the
     * sandbox user namespace.
     */
    struct {
        struct conty_user_id_map *users;
        struct conty_user_id_map *groups;
    } id_map;
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
