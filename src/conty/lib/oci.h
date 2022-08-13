#ifndef CONTY_OCI_H
#define CONTY_OCI_H

#include <unistd.h>

#include "queue.h"
#include "resource.h"

struct oci_namespace {
    char                      *ons_type;
    char                      *ons_path;
    SLIST_ENTRY(oci_namespace) ons_next;
};
void oci_namespace_free(struct oci_namespace *ns);

struct oci_id_mapping {
    unsigned int                oid_container;
    unsigned int                oid_host;
    unsigned int                oid_count;
    SLIST_ENTRY(oci_id_mapping) oid_next;
};

struct oci_rootfs {
    char *orfs_path;
    char  orfs_readonly;
};

void oci_rootfs_free(struct oci_rootfs *rootfs);

struct oci_process {
    char  *oproc_cwd;
    char **oproc_argv;
    char **oproc_envp;
};
void oci_process_free(struct oci_process *process);

struct oci_process_state {
    pid_t  opst_pid;
    char  *opst_container_id;
    char  *opst_rootfs;
    char  *opst_status;
};

void oci_process_state_free(struct oci_process_state *state);
CREATE_CLEANER(struct oci_process_state *, oci_process_state_free);

struct oci_hook {
    char                  *ohk_path;
    char                 **ohk_argv;
    char                 **ohk_envp;
    unsigned int           ohk_timeout;
    SLIST_ENTRY(oci_hook)  ohk_next;
};
void oci_hook_free(struct oci_hook *hook);

SLIST_HEAD(oci_namespaces, oci_namespace);
SLIST_HEAD(oci_ids, oci_id_mapping);
SLIST_HEAD(oci_hooks, oci_hook);

struct oci_event_hooks {
    struct oci_hooks oehk_on_runtime_create;
    struct oci_hooks oehk_on_container_created;
    struct oci_hooks oehk_on_container_start;
    struct oci_hooks oehk_on_container_started;
    struct oci_hooks oehk_on_container_stopped;
};

struct oci_conf {
    struct oci_rootfs      oc_rootfs;
    struct oci_namespaces  oc_namespaces;
    struct oci_ids         oc_uids;
    struct oci_ids         oc_gids;
    struct oci_event_hooks oc_hooks;
    struct oci_process     oc_proc;
    char                  *oc_hostname;
};

/*
 * Deserialize the buffer into an OCI configuration
 * The buffer must hold a valid JSON, otherwise NULL is returned
 */
struct oci_conf *oci_conf_deser(const char *buf);

/*
 * Deserialize an OCI configuration from the file referred to by path
 */
struct oci_conf *oci_conf_deser_file(const char *path);

/*
 * Serialize the process state into JSON
 */
char *oci_process_state_ser(const struct oci_process_state *state, size_t *len);

/*
 * Deserialize the buffer into a process state
 */
struct oci_process_state *oci_process_state_deser(const char *buf);

/*
 * Execute the given OCI hook, passing the process state into the
 * hook's standard input stream and awaiting it
 */
int oci_hook_exec(struct oci_hook *hook, const struct oci_process_state *state);

/*
 * Release the memory associated with the OCI configuration
 */
void oci_conf_free(struct oci_conf *conf);
CREATE_CLEANER(struct oci_conf *, oci_conf_free);

#endif //CONTY_OCI_H
