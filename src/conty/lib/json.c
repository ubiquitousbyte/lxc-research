#include "oci.h"

#include <string.h>

#include <json.h>

#include "log.h"
#include "resource.h"

static char *deser_str(json_object *root)
{
    size_t str_len;

    str_len = json_object_get_string_len(root);
    if (str_len <= 0)
        return NULL;

    return strndup(json_object_get_string(root), str_len);
}

static char *deser_path(json_object *root)
{
    size_t str_len;

    str_len = json_object_get_string_len(root);
    if (str_len <= 0)
        return NULL;

    return realpath(json_object_get_string(root), NULL);
}

static char **deser_strlist(json_object *root)
{
    size_t argv_len;
    json_object *cur;
    char *tmp;
    int i;
    STRINGLIST_RESOURCE char **argv = NULL;

    argv_len = json_object_array_length(root);
    if (argv_len <= 0)
        return NULL;

    argv = malloc((argv_len + 1)*sizeof(char*));
    for (i = 0; i < argv_len; i++) {
        cur = json_object_array_get_idx(root, i);

        if (!(tmp = deser_str(cur)))
            return NULL;

        argv[i] = move_ptr(tmp);
    }

    argv[i] = NULL;

    return move_ptr(argv);
}

static int deser_rootfs(json_object *root, struct oci_rootfs *rootfs)
{
    json_object *obj;

    obj = json_object_object_get(root, "path");
    if (!obj)
        return log_error_ret(-EINVAL, "oci: root filesystem path missing");

    if (!(rootfs->orfs_path = deser_path(obj)))
        return log_error_ret(-EINVAL, "oci: root filesystem path missing");

    obj = json_object_object_get(root, "readonly");
    rootfs->orfs_readonly = obj && json_object_get_boolean(obj);

    return 0;
}

static int deser_namespaces(json_object *root, struct oci_namespaces *namespaces)
{
    size_t len;
    int i;
    json_object *cur, *type, *path;
    MEM_RESOURCE char *ons_type = NULL, *ons_path = NULL;
    struct oci_namespace *ns;

    SLIST_INIT(namespaces);

    len = json_object_array_length(root);
    if (len <= 0)
        return log_error_ret(-EINVAL, "oci: namespaces missing");

    for (i = len - 1; i >= 0; i--) {
        cur = json_object_array_get_idx(root, i);

        type = json_object_object_get(cur, "type");
        if (!type)
            return log_error_ret(-EINVAL, "oci: namespace type missing");

        if (!(ons_type = deser_str(type)))
            return log_error_ret(-EINVAL, "oci: namespace type missing");

        path = json_object_object_get(cur, "path");
        if (path) {
            if (!(ons_path = deser_str(path)))
                return log_error_ret(-EINVAL, "oci: namespace path invalid");
        }

        ns = calloc(1, sizeof(struct oci_namespace));
        if (!ns)
            return log_fatal_ret(-ENOMEM, "oci: out of memory");

        ns->ons_type = move_ptr(ons_type);
        ns->ons_path = move_ptr(ons_path);

        SLIST_INSERT_HEAD(namespaces, ns, ons_next);
    }

    return 0;
}

static int deser_ids(json_object *root, struct oci_ids *uids)
{
    size_t len;
    int i;

    json_object *cur, *container_id, *host_id, *size;
    struct oci_id_mapping *idm;

    SLIST_INIT(uids);

    len = json_object_array_length(root);
    for (i = len - 1; i >= 0; i--) {
        cur = json_object_array_get_idx(root, i);

        container_id = json_object_object_get(cur, "container_id");
        if (!container_id)
            return log_error_ret(-EINVAL, "oci: invalid uid mapping");

        host_id = json_object_object_get(cur, "host_id");
        if (!host_id)
            return log_error_ret(-EINVAL, "oci: invalid uid mapping");

        size = json_object_object_get(cur, "size");
        if (!size)
            return log_error_ret(-EINVAL, "oci: invalid uid mapping");

        idm = calloc(1, sizeof(struct oci_id_mapping));
        if (!idm)
            return log_fatal_ret(-ENOMEM, "oci: out of memory");

        idm->oid_container = json_object_get_uint64(container_id);
        idm->oid_host    = json_object_get_uint64(host_id);
        idm->oid_count   = json_object_get_uint64(size);

        SLIST_INSERT_HEAD(uids, idm, oid_next);
    }

    return 0;
}

static int deser_hooks(json_object *root, struct oci_hooks *hooks)
{
    size_t len;
    int i;

    json_object *cur, *tmp;
    MEM_RESOURCE char *oh_path = NULL;
    STRINGLIST_RESOURCE char **argv = NULL, **envp = NULL;
    unsigned int timeout = 0;
    struct oci_hook *hook;

    SLIST_INIT(hooks);

    len = json_object_array_length(root);
    for (i = len - 1; i >= 0; i--) {
        cur = json_object_array_get_idx(root, i);

        tmp = json_object_object_get(cur, "path");
        if (!tmp)
            return log_error_ret(-EINVAL, "oci: hook path invalid");

        if (!(oh_path = deser_str(tmp)))
            return log_error_ret(-EINVAL, "oci: hook path invalid");

        tmp = json_object_object_get(cur, "args");
        if (tmp) {
            if (!(argv = deser_strlist(tmp)))
                return log_error_ret(-EINVAL, "oci: hook args invalid");
        }

        tmp = json_object_object_get(cur, "env");
        if (tmp) {
            if (!(envp = deser_strlist(tmp)))
                return log_error_ret(-EINVAL, "oci: hook env invalid");
        }

        tmp = json_object_object_get(cur, "timeout");
        if (tmp)
            timeout = json_object_get_uint64(tmp);

        hook = calloc(1, sizeof(struct oci_hook));
        if (!hook)
            return log_fatal_ret(-ENOMEM, "oci: out of memory");

        hook->ohk_argv    = move_ptr(argv);
        hook->ohk_envp    = move_ptr(envp);
        hook->ohk_timeout = timeout;
        hook->ohk_path    = move_ptr(oh_path);

        SLIST_INSERT_HEAD(hooks, hook, ohk_next);
    }

    return 0;
}

static int deser_event_hooks(json_object *root, struct oci_event_hooks *hooks)
{
    int err;
    json_object *tmp;
    const struct {
        const char *name;
        struct oci_hooks *hooks;
    } helper[] = {
            { .name = "on_runtime_create",  .hooks = &hooks->oehk_on_runtime_create },
            { .name = "on_container_created", .hooks = &hooks->oehk_on_container_created },
            { .name = "on_container_start",   .hooks = &hooks->oehk_on_container_start },
            { .name = "on_contaner_started", .hooks = &hooks->oehk_on_container_started },
            { .name = "on_container_stopped", .hooks = &hooks->oehk_on_container_stopped },
    };

    for (int i = 0; i < 5; i++) {
        tmp = json_object_object_get(root, helper[i].name);

        if (!tmp)
            SLIST_INIT(helper[i].hooks);
        else if ((err = deser_hooks(tmp, helper[i].hooks)) != 0)
            return err;
    }

    return 0;
}

static int deser_proc(json_object *root, struct oci_process *proc)
{
    json_object *tmp;
    STRINGLIST_RESOURCE char **argv = NULL, **envp = NULL;
    MEM_RESOURCE char *cwd = NULL;

    tmp = json_object_object_get(root, "cwd");
    if (!tmp)
        return log_error_ret(-EINVAL, "oci: process cwd invalid");

    if (!(cwd = deser_str(tmp)))
        return log_error_ret(-EINVAL, "oci: process cwd invalid");

    tmp = json_object_object_get(root, "args");
    if (!tmp)
        return log_error_ret(-EINVAL, "oci: process args invalid");

    if (!(argv = deser_strlist(tmp)))
        return log_error_ret(-EINVAL, "oci: process args invalid");

    if (!argv[0])
        return log_error_ret(-EINVAL, "oci: process binary invalid");

    tmp = json_object_object_get(root, "env");
    if (tmp) {
        if (!(envp = deser_strlist(tmp)))
            return log_error_ret(-EINVAL, "oci: process env invalid");
    }

    proc->oproc_cwd  = move_ptr(cwd);
    proc->oproc_argv = move_ptr(argv);
    proc->oproc_envp = move_ptr(envp);

    return 0;
}

static struct oci_conf *deser_conf(json_object *root)
{
    MAKE_RESOURCE(oci_conf_free) struct oci_conf *conf = NULL;

    conf = calloc(1, sizeof(struct oci_conf));
    if (!conf)
        return NULL;

    json_object *rootfs = json_object_object_get(root, "root");
    if (!rootfs)
        return log_error_ret_errno(NULL, EINVAL, "oci: root filesystem missing");

    if (deser_rootfs(rootfs, &conf->oc_rootfs) != 0)
        return NULL;

    json_object *namespaces = json_object_object_get(root, "namespaces");
    if (!namespaces)
        return log_error_ret_errno(NULL, EINVAL, "oci: namespaces missing");

    if (deser_namespaces(namespaces, &conf->oc_namespaces) != 0)
        return NULL;

    json_object *proc = json_object_object_get(root, "process");
    if (!proc)
        return log_error_ret_errno(NULL, EINVAL, "oci: process missing");

    if (deser_proc(proc, &conf->oc_proc) != 0)
        return NULL;

    json_object *uids = json_object_object_get(root, "uid_mappings");
    if (uids && deser_ids(uids, &conf->oc_uids) != 0)
        return NULL;

    json_object *gids = json_object_object_get(root, "gid_mappings");
    if (gids && deser_ids(gids, &conf->oc_gids) != 0)
        return NULL;

    json_object *hooks = json_object_object_get(root, "hooks");
    if (hooks && deser_event_hooks(hooks, &conf->oc_hooks) != 0)
        return NULL;

    json_object *hostname = json_object_object_get(root, "hostname");
    if (hostname) {
        if (!(conf->oc_hostname = deser_str(hostname)))
            return NULL;
    }

    return move_ptr(conf);
}

static struct oci_conf *deser_conf_buf(const char *buf,
                                       json_object *(*cb)(const char *buf))
{
    struct oci_conf *conf;
    json_object *root;

    root = cb(buf);
    if (!root)
        return log_error_ret(NULL, "oci: invalid json");

    conf = deser_conf(root);
    json_object_put(root);
    return conf;
}

struct oci_conf *oci_conf_deser(const char *buf)
{
    return deser_conf_buf(buf, json_tokener_parse);
}

struct oci_conf *oci_conf_deser_file(const char *path)
{
    return deser_conf_buf(path, json_object_from_file);
}

char *oci_process_state_ser(const struct oci_process_state *state, size_t *len)
{
    char *result = NULL;
    const char *tmp = NULL;
    int err;

    json_object *root = json_object_new_object();
    if (!root)
        return log_fatal_ret(NULL, "oci: out of memory");

    err = json_object_object_add(root, "pid",
                                 json_object_new_int64(state->opst_pid));
    if (err != 0)
        goto err_out;

    err = json_object_object_add(root, "id",
                                 json_object_new_string(state->opst_container_id));
    if (err != 0)
        goto err_out;

    err = json_object_object_add(root, "bundle",
                                 json_object_new_string(state->opst_rootfs));
    if (err != 0)
        goto err_out;

    err = json_object_object_add(root, "status",
                                 json_object_new_string(state->opst_status));
    if (err != 0)
        goto err_out;

    tmp = json_object_to_json_string_length(root, 0, len);
    result = strndup(tmp, *len);

    json_object_put(root);

    return result;

err_out:
    json_object_put(root);
    return log_error_ret(NULL, "oci: cannot serialise process state");
}

struct oci_process_state *deser_process_state(json_object *root)
{
    MAKE_RESOURCE(oci_process_state_free) struct oci_process_state *state = NULL;
    json_object *tmp;

    state = calloc(1, sizeof(struct oci_process_state));
    if (!state)
        return log_fatal_ret(NULL, "oci: out of memory");

    tmp = json_object_object_get(root, "pid");
    if (!tmp)
        return log_error_ret(NULL, "oci: process_state invalid pid");

    state->opst_pid = json_object_get_int(tmp);

    tmp = json_object_object_get(root, "status");
    if (!tmp)
        return log_error_ret(NULL, "oci: process_state invalid status");

    if (!(state->opst_status = deser_str(tmp)))
        return log_error_ret(NULL, "oci: process_state invalid status");

    tmp = json_object_object_get(root, "id");
    if (!tmp)
        return log_error_ret(NULL, "oci: process_state invalid id");

    if (!(state->opst_container_id = deser_str(tmp)))
        return log_error_ret(NULL, "oci: process_state invalid id");

    tmp = json_object_object_get(root, "bundle");
    if (!tmp)
        return log_error_ret(NULL, "oci: process_state invalid bundle");

    if (!(state->opst_rootfs = deser_str(tmp)))
        return log_error_ret(NULL, "oci: process_state invalid bundle");

    return move_ptr(state);
}

struct oci_process_state *oci_process_state_deser(const char *buf)
{
    struct oci_process_state *state;
    json_object *root;

    root = json_tokener_parse(buf);
    if (!root)
        return log_error_ret(NULL, "oci: invalid json");

    state = deser_process_state(root);
    json_object_put(root);
    return state;
}

void oci_namespace_free(struct oci_namespace *ns)
{
    if (ns) {
        if (ns->ons_type)
            free(ns->ons_type);
        if (ns->ons_path)
            free(ns->ons_path);
        free(ns);
        ns = NULL;
    }
}

void oci_rootfs_free(struct oci_rootfs *rootfs)
{
    if (rootfs) {
        if (rootfs->orfs_path)
            free(rootfs->orfs_path);
        free(rootfs);
        rootfs = NULL;
    }
}

void oci_process_free(struct oci_process *proc)
{
    if (proc) {
        if (proc->oproc_cwd)
            free(proc->oproc_cwd);
        stringlist_cleaner(proc->oproc_argv);
        stringlist_cleaner(proc->oproc_envp);
        free(proc);
        proc = NULL;
    }
}

void oci_process_state_free(struct oci_process_state *state)
{
    if (state) {
        if (state->opst_rootfs)
            free(state->opst_rootfs);
        if (state->opst_container_id)
            free(state->opst_container_id);
        if (state->opst_status)
            free(state->opst_status);
        free(state);
        state = NULL;
    }
}

void oci_hook_free(struct oci_hook *hook)
{
    if (hook) {
        if (hook->ohk_path)
            free(hook->ohk_path);
        stringlist_cleaner(hook->ohk_argv);
        stringlist_cleaner(hook->ohk_envp);
        free(hook);
        hook = NULL;
    }
}

void oci_conf_free(struct oci_conf *conf)
{
    if (conf) {
        if (conf->oc_rootfs.orfs_path)
            free(conf->oc_rootfs.orfs_path);

        if (conf->oc_proc.oproc_cwd)
            free(conf->oc_proc.oproc_cwd);
        stringlist_cleaner(conf->oc_proc.oproc_argv);
        stringlist_cleaner(conf->oc_proc.oproc_envp);

        if (conf->oc_hostname)
            free(conf->oc_hostname);

        LIST_CLEAN(&conf->oc_namespaces, ons_next, oci_namespace_free);
        LIST_CLEAN(&conf->oc_hooks.oehk_on_runtime_create, ohk_next, oci_hook_free);
        LIST_CLEAN(&conf->oc_hooks.oehk_on_container_created, ohk_next, oci_hook_free);
        LIST_CLEAN(&conf->oc_hooks.oehk_on_container_start, ohk_next, oci_hook_free);
        LIST_CLEAN(&conf->oc_hooks.oehk_on_container_started, ohk_next, oci_hook_free);
        LIST_CLEAN(&conf->oc_hooks.oehk_on_container_stopped, ohk_next, oci_hook_free);
        LIST_CLEAN(&conf->oc_uids, oid_next, free);
        LIST_CLEAN(&conf->oc_gids, oid_next, free);

        free(conf);
    }
}