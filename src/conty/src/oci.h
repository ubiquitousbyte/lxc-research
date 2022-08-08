#ifndef CONTY_OCI_H
#define CONTY_OCI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "queue.h"

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

struct oci_runtime_ops {
    struct oci_process_state *(*get_state)(const char *container_id);
    int                       (*create_container)(const char *container_id,
                                                  const char *bundle);
    int                       (*start_container)(const char *container_id);
    int                       (*kill_container)(const char *container_id,
                                                int signal);
    int                       (*delete_container)(const char *container_id);
};

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_OCI_H
