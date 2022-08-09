#ifndef CONTY_CONTY_H
#define CONTY_CONTY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include <conty/queue.h>

/*
 * Open Containers Initiative (OCI) types defined in partial accordance with
 * the runtime specification:
 *  https://github.com/opencontainers/runtime-spec/blob/main/config-linux.md
 */
struct oci_namespace {
    char                     *ons_type;
    char                     *ons_path;
    LIST_ENTRY(oci_namespace) ons_next;
};

struct oci_id_mapping {
    unsigned int               oid_container;
    unsigned int               oid_host;
    unsigned int               oid_count;
    LIST_ENTRY(oci_id_mapping) oid_next;
};

struct oci_device {
    char                   *odev_path;
    char                   *odev_type;
    unsigned long           odev_major;
    unsigned long           odev_minor;
    unsigned int            odev_fmode;
    unsigned int            odev_uowner;
    unsigned int            odev_gowner;
    LIST_ENTRY(oci_device)  odev_next;
};

struct oci_rootfs {
    char *ocirfs_path;
    char  ocirfs_ro;
};

struct oci_process {
    char  *oproc_cwd;
    char **oproc_argv;
    char **oproc_envp;
};

struct oci_process_state {
    pid_t       oprocst_sbpid;
    const char *oprocst_sbid;
    const char *oprocst_rootfs;
    const char *oprocst_status;
};

struct oci_hook {
    char                 *oh_path;
    char                **oh_argv;
    char                **oh_envp;
    unsigned int          oh_timeout;
    LIST_ENTRY(oci_hook)  oh_next;
};

LIST_HEAD(oci_namespaces, oci_namespace);
LIST_HEAD(oci_uids, oci_id_mapping);
LIST_HEAD(oci_devices, oci_device);
LIST_HEAD(oci_hooks, oci_hook);

struct oci_event_hooks {
    struct oci_hooks oeh_rt_create;
    struct oci_hooks oeh_sb_created;
    struct oci_hooks oeh_sb_start;
    struct oci_hooks oeh_sb_started;
    struct oci_hooks oeh_sb_stopped;
};

struct oci_conf {
    struct oci_rootfs      oc_rootfs;
    struct oci_namespaces  oc_namespaces;
    struct oci_uids        oc_uids;
    struct oci_uids        oc_gids;
    struct oci_devices     oc_devices;
    struct oci_event_hooks oc_hooks;
    struct oci_process     oc_proc;
    char                  *oc_hostname;
};

struct oci_conf *oci_deser_conf(const char *buf);
struct oci_conf *oci_deser_conf_file(const char *path);
void oci_conf_free(struct oci_conf *conf);

char *oci_ser_process_state(const struct oci_process_state *state,
                            size_t *buf_len);
struct oci_process_state *oci_deser_process_state(const char *buf);

struct oci_process_state *oci_rt_get_state(const char *container_id);
int oci_rt_create_container(const char *container_id, const char *bundle_path);
int oci_rt_start_container(const char *container_id);
int oci_rt_kill_container(const char *container_id, int signal);
int oci_rt_delete_container(const char *container_id);

typedef enum {
    CONTAINER_CREATING     = 0,
    CONTAINER_CREATED      = 1,
    CONTAINER_RUNNING      = 2,
    CONTAINER_STOPPED      = 3,
} oci_container_status_t;

#define OCI_CONTAINER_STATUS_MAX (CONTAINER_STOPPED)

static const char *oci_container_statuses[OCI_CONTAINER_STATUS_MAX + 1] = {
        [CONTAINER_CREATING] = "creating",
        [CONTAINER_CREATED]  = "created",
        [CONTAINER_RUNNING]  = "running",
        [CONTAINER_STOPPED]  = "stopped",
};

#define CONTY_HOOK_DEFAULT_TIMEOUT 2

/*
 * Custom types, functions and implementation of the OCI runtime spec
 */
int conty_hook_exec(const char *prog, const char *argv[], const char *envp[],
                    const char *buf, size_t buf_len, int timeout);

int conty_oci_hook_exec(const struct oci_hook *hook,
                        const struct oci_process_state *state);

int conty_oci_write_uid_map(const struct oci_uids *uids);
int conty_oci_write_gid_map(const struct oci_uids *gids);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_CONTY_H
