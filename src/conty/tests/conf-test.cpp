#include <gtest/gtest.h>

#include "oci.h"

#include <string.h>

TEST(oci_conf, from_json_str)
{
    int i;
    struct oci_conf *conf = NULL;
    std::string conf_string = R"({
        "process": {
            "args": [
                "sh",
                "echo",
                "lol"
            ],
            "cwd": "/bin"
        },
        "root": {
            "path": "/root",
            "readonly": false
        },
        "namespaces": [
            {
                "type": "user",
                "path": "/proc/1234/ns/user"
            },
            {
                "type": "net"
            },
            {
                "type": "ipc"
            },
            {
                "type": "mnt"
            },
            {
                "type": "uts"
            }
        ],
        "uid_mappings": [
            {
                "sandbox_id": 0,
                "host_id": 1000,
                "size": 1
            }
        ],
        "gid_mappings": [
            {
                "sandbox_id": 0,
                "host_id": 1000,
                "size": 1
            }
        ],
        "hostname": "sandboxy",
        "hooks": {
            "on_runtime_create": [
                {
                    "path": "/usr/bin/fix-mounts",
                    "args": ["fix-mounts", "arg1", "arg2"],
                    "env":  [ "key1=value1"]
                },
                {
                    "path": "/usr/bin/setup-network"
                }
            ]
        },
        "devices": [
            {
                "path": "/dev/fuse",
                "type": "c",
                "major": 10,
                "minor": 229,
                "uid": 0,
                "gid": 0
            },
            {
                "path": "/dev/sda",
                "type": "b",
                "major": 8,
                "minor": 0,
                "uid": 0,
                "gid": 0
            }
        ]
    })";


    conf = oci_deser_conf(conf_string.c_str());
    EXPECT_TRUE(conf != NULL);

    EXPECT_STREQ(conf->oc_proc.oproc_cwd, "/bin");
    const char *expected_proc_args[] = { "sh", "echo", "lol", (char *) NULL };
    for (i = 0; i < 4; i++)
        EXPECT_STREQ(conf->oc_proc.oproc_argv[i], expected_proc_args[i]);

    EXPECT_STREQ(conf->oc_rootfs.ocirfs_path, "/root");
    EXPECT_EQ(conf->oc_rootfs.ocirfs_ro, 0);

    const struct oci_namespace expected_namespaces[] = {
            { .ons_type = "user", .ons_path = "/proc/1234/ns/user" },
            { .ons_type = "net",  .ons_path = NULL },
            { .ons_type = "ipc",  .ons_path = NULL },
            { .ons_type = "mnt",  .ons_path = NULL },
            { .ons_type = "uts",  .ons_path = NULL },
    };

    i = 0;
    struct oci_namespace *cur_ns, *tmp_ns;
    LIST_FOREACH_SAFE(cur_ns, &conf->oc_namespaces, ons_next, tmp_ns) {
        EXPECT_STREQ(cur_ns->ons_path, expected_namespaces[i].ons_path);
        EXPECT_STREQ(cur_ns->ons_type, expected_namespaces[i].ons_type);
        ++i;
    }
    EXPECT_EQ(i, 5);

    i = 0;
    const struct oci_id_mapping expected_uid_mappings[] = {
            { .oid_container = 0, .oid_host = 1000, .oid_count = 1 }
    };
    struct oci_id_mapping *cur_id, *tmp_id;
    LIST_FOREACH_SAFE(cur_id, &conf->oc_uids, oid_next, tmp_id) {
        EXPECT_EQ(cur_id->oid_count, expected_uid_mappings[i].oid_count);
        EXPECT_EQ(cur_id->oid_host, expected_uid_mappings[i].oid_host);
        EXPECT_EQ(cur_id->oid_container, expected_uid_mappings[i].oid_container);
        ++i;
    }
    EXPECT_EQ(i, 1);

    i = 0;
    LIST_FOREACH_SAFE(cur_id, &conf->oc_gids, oid_next, tmp_id) {
        EXPECT_EQ(cur_id->oid_count, expected_uid_mappings[i].oid_count);
        EXPECT_EQ(cur_id->oid_host, expected_uid_mappings[i].oid_host);
        EXPECT_EQ(cur_id->oid_container, expected_uid_mappings[i].oid_container);
        ++i;
    }
    EXPECT_EQ(i, 1);

    EXPECT_STREQ(conf->oc_hostname, "sandboxy");

    i = 0;
    const struct oci_device expected_devices[] = {
            {
                    .odev_path = "/dev/fuse",
                    .odev_type = "c",
                    .odev_major = 10,
                    .odev_minor = 229,
                    .odev_uowner = 0,
                    .odev_gowner = 0
            },
            {
                    .odev_path = "/dev/sda",
                    .odev_type = "b",
                    .odev_major = 8,
                    .odev_minor = 0,
                    .odev_uowner = 0,
                    .odev_gowner = 0
            }
    };
    struct oci_device *cur_dev, *tmp_dev;
    LIST_FOREACH_SAFE(cur_dev, &conf->oc_devices, odev_next, tmp_dev) {
        EXPECT_STREQ(cur_dev->odev_path, expected_devices[i].odev_path);
        EXPECT_STREQ(cur_dev->odev_type, expected_devices[i].odev_type);
        EXPECT_EQ(cur_dev->odev_major, expected_devices[i].odev_major);
        EXPECT_EQ(cur_dev->odev_minor, expected_devices[i].odev_minor);
        EXPECT_EQ(cur_dev->odev_uowner, expected_devices[i].odev_uowner);
        EXPECT_EQ(cur_dev->odev_gowner, expected_devices[i].odev_gowner);
        i++;
    }
    EXPECT_EQ(i, 2);

    const char *argv[] = { "fix-mounts", "arg1", "arg2", (char *) NULL };
    const char *envp[] = { "key1=value1", (char *) NULL };
    const struct oci_hook expected_hooks[] = {
            {
                .oh_path = "/usr/bin/fix-mounts",
                .oh_argv = const_cast<char **>(argv),
                .oh_envp = const_cast<char **>(envp)
            },
            {
                .oh_path = "/usr/bin/setup-network",
                .oh_argv = NULL,
                .oh_envp = NULL
            }
    };
    i = 0;
    struct oci_hook *cur_hook, *tmp_hook;
    LIST_FOREACH_SAFE(cur_hook, &conf->oc_hooks.oeh_rt_create, oh_next, tmp_hook) {
        EXPECT_STREQ(cur_hook->oh_path, expected_hooks[i].oh_path);
        i++;
    }
    EXPECT_EQ(i, 2);

    oci_conf_free(conf);
}