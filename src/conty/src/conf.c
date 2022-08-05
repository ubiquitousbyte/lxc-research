#include "conf.h"

#include <string.h>
#include <json.h>

#include "log.h"

static void conty_json_object_deref_function(json_object **object)
{
    if (*object) {
        json_object_put(*object);
        *object = NULL;
    }
}

static struct conty_conf *__conty_conf_parse(json_object *root)
{
    CONTY_INVOKE_CLEANER(conty_conf_free) struct conty_conf *conf;
    size_t tmp_len;

    conf = calloc(1, sizeof(struct conty_conf));
    if (!conf)
        return LOG_FATAL_RET(NULL, "conty_conf: no memory");

    json_object *sandbox_id = json_object_object_get(root, "sandbox_id");
    if (!sandbox_id)
        return LOG_ERROR_RET(NULL, "conty_conf: sandbox_id missing");

    tmp_len = json_object_get_string_len(sandbox_id);
    if (tmp_len < 1)
        return LOG_ERROR_RET(NULL, "conty_conf: sandbox_id is empty");

    conf->sandbox_id = strndup(json_object_get_string(sandbox_id), tmp_len);

    json_object *rootfs = json_object_object_get(root, "rootfs");
    if (!rootfs)
        return LOG_ERROR_RET(NULL, "conty_conf: rootfs missing");

    tmp_len = json_object_get_string_len(rootfs);
    if (tmp_len < 1)
        return LOG_ERROR_RET(NULL, "conty_conf: rootfs is empty");

    conf->rootfs = strndup(json_object_get_string(rootfs), tmp_len);

    json_object *netconf = json_object_object_get(root, "net");
    if (!netconf)
        return LOG_ERROR_RET(NULL, "conty_conf: network configuration missing");

    json_object *netconf_vethip = json_object_object_get(netconf, "veth_ip");
    if (!netconf_vethip)
        return LOG_ERROR_RET(NULL, "conty_conf: veth ip missing");

    tmp_len = json_object_get_string_len(netconf_vethip);
    if (tmp_len < 1)
        return LOG_ERROR_RET(NULL, "conty_conf: veth_ip is empty");

    conf->net.veth_ip = strndup(json_object_get_string(netconf_vethip), tmp_len);

    json_object  *netconf_bridgeip = json_object_object_get(netconf, "bridge_ip");
    if (!netconf_bridgeip)
        return LOG_ERROR_RET(NULL, "conty_conf: bridge ip is missing");

    tmp_len = json_object_get_string_len(netconf_bridgeip);
    if (tmp_len < 1)
        return LOG_ERROR_RET(NULL, "conty_conf: bridge_ip is empty");

    conf->net.bridge_ip = strndup(json_object_get_string(netconf_bridgeip),
                                  tmp_len);

    return CONTY_MOVE_PTR(conf);
}

struct conty_conf *conty_conf_from_file(const char *path)
{
    CONTY_INVOKE_CLEANER(conty_json_object_deref) json_object *root = NULL;

    root = json_object_from_file(path);
    if (!root)
        return LOG_ERROR_RET(NULL, "conty_conf: config file %s not found", path);

    return __conty_conf_parse(root);
}

struct conty_conf *conty_conf_from_string(const char *string)
{
    CONTY_INVOKE_CLEANER(conty_json_object_deref) json_object *root = NULL;

    root = json_tokener_parse(string);
    if (!root)
        return LOG_ERROR_RET(NULL, "conty_conf: invalid json detected");

    return __conty_conf_parse(root);
}

void conty_conf_free(struct conty_conf *conf)
{
    if (conf) {
        if (conf->sandbox_id)
            free(conf->sandbox_id);
        if (conf->rootfs)
            free(conf->rootfs);
        if (conf->net.bridge_ip)
            free(conf->net.bridge_ip);
        if (conf->net.veth_ip)
            free(conf->net.veth_ip);
        free(conf);
    }
}