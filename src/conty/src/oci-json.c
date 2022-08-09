#include <conty/conty.h>

#include <errno.h>
#include <string.h>

#include "log.h"
#include "resource.h"

#include <json.h>

void oci_namespaces_free(struct oci_namespaces *namespaces)
{
    struct oci_namespace *head;
    while (!LIST_EMPTY(namespaces)) {
        head = LIST_FIRST(namespaces);
        LIST_REMOVE(head, ons_next);
        if (head->ons_type)
            free(head->ons_type);
        if (head->ons_path)
            free(head->ons_path);
        free(head);
        head = NULL;
    }
}

void oci_devices_free(struct oci_devices *devices)
{
    struct oci_device *head;
    while (!LIST_EMPTY(devices)) {
        head = LIST_FIRST(devices);
        LIST_REMOVE(head, odev_next);
        if (head->odev_path)
            free(head->odev_path);
        if (head->odev_type)
            free(head->odev_type);
        free(head);
        head = NULL;
    }
}

void oci_hooks_free(struct oci_hooks *hooks)
{
    struct oci_hook *head;
    while (!LIST_EMPTY(hooks)) {
        head = LIST_FIRST(hooks);
        LIST_REMOVE(head, oh_next);
        if (head->oh_path)
            free(head->oh_path);
        conty_free_strings(head->oh_argv);
        conty_free_strings(head->oh_envp);
        free(head);
        head = NULL;
    }
}

void oci_process_free(struct oci_process *proc)
{
    if (proc) {
        if (proc->oproc_cwd)
            free(proc->oproc_cwd);
        conty_free_strings(proc->oproc_argv);
        conty_free_strings(proc->oproc_envp);
    }
}

void oci_uids_free(struct oci_uids *ids)
{
    struct oci_id_mapping *head;
    while (!LIST_EMPTY(ids)) {
        head = LIST_FIRST(ids);
        LIST_REMOVE(head, oid_next);
        free(head);
        head = NULL;
    }
}

void oci_conf_free(struct oci_conf *conf)
{
    if (conf) {
        if (conf->oc_rootfs.ocirfs_path)
            free(conf->oc_rootfs.ocirfs_path);

        if (conf->oc_hostname)
            free(conf->oc_hostname);

        oci_namespaces_free(&conf->oc_namespaces);
        oci_devices_free(&conf->oc_devices);
        oci_hooks_free(&conf->oc_hooks.oeh_rt_create);
        oci_hooks_free(&conf->oc_hooks.oeh_sb_created);
        oci_hooks_free(&conf->oc_hooks.oeh_sb_start);
        oci_hooks_free(&conf->oc_hooks.oeh_sb_started);
        oci_hooks_free(&conf->oc_hooks.oeh_sb_stopped);
        oci_uids_free(&conf->oc_uids);
        oci_uids_free(&conf->oc_gids);
        oci_process_free(&conf->oc_proc);
        free(conf);
    }
}

static char *oci_deser_json_str(json_object *root)
{
    size_t str_len;

    str_len = json_object_get_string_len(root);
    if (str_len <= 0)
        return NULL;

    return strndup(json_object_get_string(root), str_len);
}

static char **oci_deser_json_strlist(json_object *root)
{
    size_t argv_len;
    json_object *cur;
    char *tmp;
    int i;
    __CONTY_FREE_STRLIST char **argv = NULL;

    argv_len = json_object_array_length(root);
    if (argv_len <= 0)
        return NULL;

    argv = malloc((argv_len + 1)*sizeof(char*));
    for (i = 0; i < argv_len; i++) {
        cur = json_object_array_get_idx(root, i);

        if (!(tmp = oci_deser_json_str(cur)))
            return NULL;

        argv[i] = CONTY_MOVE_PTR(tmp);
    }

    argv[i] = NULL;

    return CONTY_MOVE_PTR(argv);
}

static int oci_deser_json_rootfs(json_object *root, struct oci_rootfs *rootfs)
{
    json_object *path, *ro;

    path = json_object_object_get(root, "path");
    if (!path)
        return LOG_ERROR_RET(-EINVAL, "oci: root filesystem path missing");

    if (!(rootfs->ocirfs_path = oci_deser_json_str(path)))
        return LOG_ERROR_RET(-EINVAL, "oci: root filesystem path missing");

    ro = json_object_object_get(root, "readonly");
    if (ro && json_object_get_boolean(ro))
        rootfs->ocirfs_ro = 1;
    else
        rootfs->ocirfs_ro = 0;

    return 0;
}

static int oci_deser_json_namespaces(json_object *root,
                                     struct oci_namespaces *namespaces)
{
    size_t len;
    int i;
    json_object *cur, *type, *path;
    __CONTY_FREE char *ons_type = NULL, *ons_path = NULL;
    struct oci_namespace *ns;

    LIST_INIT(namespaces);

    len = json_object_array_length(root);
    if (len <= 0)
        return LOG_ERROR_RET(-EINVAL, "oci: namespaces missing");

    for (i = len - 1; i >= 0; i--) {
        cur = json_object_array_get_idx(root, i);

        type = json_object_object_get(cur, "type");
        if (!type)
            return LOG_ERROR_RET(-EINVAL, "oci: namespace type missing");

        if (!(ons_type = oci_deser_json_str(type)))
            return LOG_ERROR_RET(-EINVAL, "oci: namespace type missing");

        path = json_object_object_get(cur, "path");
        if (path) {
            if (!(ons_path = oci_deser_json_str(path)))
                return LOG_ERROR_RET(-EINVAL, "oci: namespace path invalid");
        }

        ns = calloc(1, sizeof(struct oci_namespace));
        if (!ns)
            return LOG_FATAL_RET(-ENOMEM, "oci: out of memory");

        ns->ons_type = CONTY_MOVE_PTR(ons_type);
        ns->ons_path = CONTY_MOVE_PTR(ons_path);

        LIST_INSERT_HEAD(namespaces, ns, ons_next);
    }

    return 0;
}

static int oci_deser_json_uids(json_object *root, struct oci_uids *uids)
{
    size_t len;
    int i;

    json_object *cur, *sandbox_id, *host_id, *size;
    struct oci_id_mapping *idm;

    LIST_INIT(uids);

    len = json_object_array_length(root);
    for (i = len - 1; i >= 0; i--) {
        cur = json_object_array_get_idx(root, i);

        sandbox_id = json_object_object_get(cur, "sandbox_id");
        if (!sandbox_id)
            return LOG_ERROR_RET(-EINVAL, "oci: invalid uid mapping");

        host_id = json_object_object_get(cur, "host_id");
        if (!host_id)
            return LOG_ERROR_RET(-EINVAL, "oci: invalid uid mapping");

        size = json_object_object_get(cur, "size");
        if (!size)
            return LOG_ERROR_RET(-EINVAL, "oci: invalid uid mapping");

        idm = calloc(1, sizeof(struct oci_id_mapping));
        if (!idm)
            return LOG_FATAL_RET(-ENOMEM, "oci: out of memory");

        idm->oid_container = json_object_get_uint64(sandbox_id);
        idm->oid_host    = json_object_get_uint64(host_id);
        idm->oid_count   = json_object_get_uint64(size);

        LIST_INSERT_HEAD(uids, idm, oid_next);
    }

    return 0;
}

static int oci_deser_json_devices(json_object *root, struct oci_devices *devs)
{
    size_t len;
    int i;
    json_object *cur, *tmp;
    unsigned long major, minor;
    unsigned int uid = 65534, gid = 65534;
    struct oci_device *dev;
    __CONTY_FREE char *odev_path = NULL, *odev_type = NULL;

    LIST_INIT(devs);

    len = json_object_array_length(root);
    for (i = len - 1; i >= 0; i--) {
        cur = json_object_array_get_idx(root, i);

        tmp = json_object_object_get(cur, "path");
        if (!tmp)
            return LOG_ERROR_RET(-EINVAL, "oci: invalid device path");

        if (!(odev_path = oci_deser_json_str(tmp)))
            return LOG_ERROR_RET(-EINVAL, "oci: invalid device path");

        tmp = json_object_object_get(cur, "type");
        if (!tmp)
            return LOG_ERROR_RET(-EINVAL, "oci: invalid device type");

        if (!(odev_type = oci_deser_json_str(tmp)))
            return LOG_ERROR_RET(-EINVAL, "oci: invalid device type");

        if (strncmp("p", odev_type, 1) != 0) {
            tmp = json_object_object_get(cur, "major");
            if (!tmp)
                return LOG_ERROR_RET(-EINVAL, "oci: invalid device major num");

            major = json_object_get_uint64(tmp);

            tmp = json_object_object_get(cur, "minor");
            if (!tmp)
                return LOG_ERROR_RET(-EINVAL, "oci: invalid device minor");

            minor = json_object_get_uint64(tmp);

            tmp = json_object_object_get(cur, "uid");
            if (tmp)
                uid = json_object_get_int(tmp);
        }

        tmp = json_object_object_get(cur, "gid");
        if (tmp)
            gid = json_object_get_int(tmp);

        dev = calloc(1, sizeof(struct oci_device));
        if (!dev)
            return LOG_FATAL_RET(-ENOMEM, "oci: out of memory");

        dev->odev_path = CONTY_MOVE_PTR(odev_path);
        dev->odev_type = CONTY_MOVE_PTR(odev_type);
        dev->odev_uowner = uid;
        dev->odev_gowner = gid;
        dev->odev_major = major;
        dev->odev_minor = minor;

        LIST_INSERT_HEAD(devs, dev, odev_next);
    }

    return 0;
}

static int oci_deser_json_hooks(json_object *root, struct oci_hooks *hooks)
{
    size_t len;
    int i;

    json_object *cur, *tmp;
    __CONTY_FREE char *oh_path = NULL;
    __CONTY_FREE_STRLIST char **argv = NULL, **envp = NULL;
    unsigned int timeout = 0;
    struct oci_hook *hook;

    LIST_INIT(hooks);

    len = json_object_array_length(root);
    for (i = len - 1; i >= 0; i--) {
        cur = json_object_array_get_idx(root, i);

        tmp = json_object_object_get(cur, "path");
        if (!tmp)
            return LOG_ERROR_RET(-EINVAL, "oci: hook path invalid");

        if (!(oh_path = oci_deser_json_str(tmp)))
            return LOG_ERROR_RET(-EINVAL, "oci: hook path invalid");

        tmp = json_object_object_get(cur, "args");
        if (tmp) {
            if (!(argv = oci_deser_json_strlist(tmp)))
                return LOG_ERROR_RET(-EINVAL, "oci: hook args invalid");
        }

        tmp = json_object_object_get(cur, "env");
        if (tmp) {
            if (!(envp = oci_deser_json_strlist(tmp)))
                return LOG_ERROR_RET(-EINVAL, "oci: hook env invalid");
        }

        tmp = json_object_object_get(cur, "timeout");
        if (tmp)
            timeout = json_object_get_uint64(tmp);

        hook = calloc(1, sizeof(struct oci_hook));
        if (!hook)
            return LOG_FATAL_RET(-ENOMEM, "oci: out of memory");

        hook->oh_argv = CONTY_MOVE_PTR(argv);
        hook->oh_envp = CONTY_MOVE_PTR(envp);
        hook->oh_timeout = timeout;
        hook->oh_path = CONTY_MOVE_PTR(oh_path);

        LIST_INSERT_HEAD(hooks, hook, oh_next);
    }

    return 0;
}

static int oci_deser_json_event_hooks(json_object *root,
                                      struct oci_event_hooks *hooks)
{
    int err;
    json_object *tmp;
    const struct {
        const char *name;
        struct oci_hooks *hooks;
    } helper[] = {
            { .name = "on_runtime_create",  .hooks = &hooks->oeh_rt_create },
            { .name = "on_sandbox_created", .hooks = &hooks->oeh_sb_created },
            { .name = "on_sandbox_start",   .hooks = &hooks->oeh_sb_start },
            { .name = "on_sandbox_started", .hooks = &hooks->oeh_sb_started },
            { .name = "on_sandbox_stopped", .hooks = &hooks->oeh_sb_stopped },
    };

    for (int i = 0; i < 5; i++) {
        tmp = json_object_object_get(root, helper[i].name);

        if (!tmp)
            LIST_INIT(helper[i].hooks);
        else if ((err = oci_deser_json_hooks(tmp, helper[i].hooks)) != 0)
            return err;
    }

    return 0;
}

static int oci_deser_json_process(json_object *root,
                                  struct oci_process *proc)
{
    json_object *tmp;
    __CONTY_FREE_STRLIST char **argv = NULL, **envp = NULL;
    __CONTY_FREE char *cwd = NULL;

    tmp = json_object_object_get(root, "cwd");
    if (!tmp)
        return LOG_ERROR_RET(-EINVAL, "oci: process cwd invalid");

    if (!(cwd = oci_deser_json_str(tmp)))
        return LOG_ERROR_RET(-EINVAL, "oci: process cwd invalid");

    tmp = json_object_object_get(root, "args");
    if (!tmp)
        return LOG_ERROR_RET(-EINVAL, "oci: process args invalid");

    if (!(argv = oci_deser_json_strlist(tmp)))
        return LOG_ERROR_RET(-EINVAL, "oci: process args invalid");

    if (!argv[0])
        return LOG_ERROR_RET(-EINVAL, "oci: process binary invalid");

    tmp = json_object_object_get(root, "env");
    if (tmp) {
        if (!(envp = oci_deser_json_strlist(tmp)))
            return LOG_ERROR_RET(-EINVAL, "oci: process env invalid");
    }

    proc->oproc_cwd  = CONTY_MOVE_PTR(cwd);
    proc->oproc_argv = CONTY_MOVE_PTR(argv);
    proc->oproc_envp = CONTY_MOVE_PTR(envp);

    return 0;
}

CONTY_CREATE_CLEANUP_FUNC(struct oci_conf *, oci_conf_free);
static struct oci_conf *__oci_deser_json_conf(json_object *root)
{
    CONTY_INVOKE_CLEANER(oci_conf_free) struct oci_conf *conf = NULL;

    conf = calloc(1, sizeof(struct oci_conf));
    if (!conf)
        return NULL;

    json_object *rootfs = json_object_object_get(root, "root");
    if (!rootfs)
        return LOG_ERROR_RET_ERRNO(NULL, EINVAL, "oci: root filesystem missing");

    if (oci_deser_json_rootfs(rootfs, &conf->oc_rootfs) != 0)
        return NULL;

    json_object *namespaces = json_object_object_get(root, "namespaces");
    if (!namespaces)
        return LOG_ERROR_RET_ERRNO(NULL, EINVAL, "oci: namespaces missing");

    if (oci_deser_json_namespaces(namespaces, &conf->oc_namespaces) != 0)
        return NULL;

    json_object *proc = json_object_object_get(root, "process");
    if (!proc)
        return LOG_ERROR_RET_ERRNO(NULL, EINVAL, "oci: process missing");

    if (oci_deser_json_process(proc, &conf->oc_proc) != 0)
        return NULL;

    json_object *uids = json_object_object_get(root, "uid_mappings");
    if (uids && oci_deser_json_uids(uids, &conf->oc_uids) != 0)
        return NULL;

    json_object *gids = json_object_object_get(root, "gid_mappings");
    if (gids && oci_deser_json_uids(gids, &conf->oc_gids) != 0)
        return NULL;

    json_object *devices = json_object_object_get(root, "devices");
    if (devices && oci_deser_json_devices(devices, &conf->oc_devices) != 0)
        return NULL;

    json_object *hooks = json_object_object_get(root, "hooks");
    if (hooks && oci_deser_json_event_hooks(hooks, &conf->oc_hooks) != 0)
        return NULL;

    json_object *hostname = json_object_object_get(root, "hostname");
    if (hostname) {
        if (!(conf->oc_hostname = oci_deser_json_str(hostname)))
            return NULL;
    }

    return CONTY_MOVE_PTR(conf);
}

char *oci_ser_process_state(const struct oci_process_state *state,
                                 size_t *buf_len)
{
    char *result = NULL;
    const char *tmp = NULL;
    int err;

    json_object *root = json_object_new_object();
    if (!root)
        return LOG_FATAL_RET(NULL, "oci: out of memory");

    err = json_object_object_add(root, "pid",
                                 json_object_new_int64(state->oprocst_sbpid));
    if (err != 0)
        goto err_out;

    err = json_object_object_add(root, "id",
                                 json_object_new_string(state->oprocst_sbid));
    if (err != 0)
        goto err_out;

    err = json_object_object_add(root, "bundle",
                                 json_object_new_string(state->oprocst_rootfs));
    if (err != 0)
        goto err_out;

    err = json_object_object_add(root, "status",
                                 json_object_new_string(state->oprocst_status));
    if (err != 0)
        goto err_out;

    tmp = json_object_to_json_string_length(root, 0, buf_len);
    result = strndup(tmp, *buf_len);

    json_object_put(root);

    return result;
err_out:
    json_object_put(root);
    return LOG_ERROR_RET(NULL, "oci: cannot serialise process state");
}

struct oci_process_state *oci_deser_json_process_state(json_object *root)
{
    __CONTY_FREE struct oci_process_state *state = NULL;
    json_object *tmp;

    state = calloc(1, sizeof(struct oci_process_state));
    if (!state)
        return LOG_FATAL_RET(NULL, "oci: out of memory");

    tmp = json_object_object_get(root, "pid");
    if (!tmp)
        return LOG_ERROR_RET(NULL, "oci: process_state invalid pid");

    state->oprocst_sbpid = json_object_get_int(tmp);

    tmp = json_object_object_get(root, "status");
    if (!tmp)
        return LOG_ERROR_RET(NULL, "oci: process_state invalid status");

    if (!(state->oprocst_status = oci_deser_json_str(tmp)))
        return LOG_ERROR_RET(NULL, "oci: process_state invalid status");

    tmp = json_object_object_get(root, "id");
    if (!tmp)
        return LOG_ERROR_RET(NULL, "oci: process_state invalid id");

    if (!(state->oprocst_sbid = oci_deser_json_str(tmp)))
        return LOG_ERROR_RET(NULL, "oci: process_state invalid id");

    tmp = json_object_object_get(root, "bundle");
    if (!tmp)
        return LOG_ERROR_RET(NULL, "oci: process_state invalid bundle");

    if (!(state->oprocst_rootfs = oci_deser_json_str(tmp)))
        return LOG_ERROR_RET(NULL, "oci: process_state invalid bundle");

    return CONTY_MOVE_PTR(state);
}

struct oci_process_state *oci_deser_process_state(const char *buf)
{
    struct oci_process_state *state;
    json_object *root;

    root = json_tokener_parse(buf);
    if (!root)
        return LOG_ERROR_RET(NULL, "oci: invalid json");

    state = oci_deser_json_process_state(root);
    json_object_put(root);
    return state;
}

struct oci_conf *oci_deser_conf(const char *buf)
{
    struct oci_conf *conf;
    json_object *root;

    root = json_tokener_parse(buf);
    if (!root)
        return LOG_ERROR_RET(NULL, "oci: invalid json");

    conf = __oci_deser_json_conf(root);
    json_object_put(root);
    return conf;
}

struct oci_conf *oci_deser_conf_file(const char *path)
{
    struct oci_conf *conf;
    json_object *root;

    root = json_object_from_file(path);
    if (!root)
        return LOG_ERROR_RET(NULL, "oci: invalid json");

    conf = __oci_deser_json_conf(root);
    json_object_put(root);
    return conf;
}