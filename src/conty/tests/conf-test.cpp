#include <gtest/gtest.h>

#include "conf.h"

#include <string.h>

TEST(conty_conf, success)
{
    struct conty_conf *conf = NULL;
    std::string conf_string = R"({
        "sandbox_id": "first-container-ever",
        "rootfs": "/some/directory",
        "net": {
            "veth_ip": "192.168.0.101",
            "bridge_ip": "192.168.0.102",
         }
    })";

    conf = conty_conf_from_string(conf_string.c_str());

    EXPECT_TRUE(conf != NULL);
    EXPECT_STREQ(conf->sandbox_id, "first-container-ever");
    EXPECT_STREQ(conf->rootfs, "/some/directory");
    EXPECT_STREQ(conf->net.veth_ip, "192.168.0.101");
    EXPECT_STREQ(conf->net.bridge_ip, "192.168.0.102");
}

TEST(conty_conf, net_missing)
{
    struct conty_conf *conf = NULL;
    std::string conf_string = R"({
        "sandbox_id": "first-container-ever",
        "rootfs": "/some/directory",
    })";

    conf = conty_conf_from_string(conf_string.c_str());

    EXPECT_TRUE(conf == NULL);
}

TEST(conty_conf, net_veth_ip_missing)
{
    struct conty_conf *conf = NULL;
    std::string conf_string = R"({
        "sandbox_id": "first-container-ever",
        "rootfs": "/some/directory",
        "net": {
            "bridge_ip": "192.168.0.102",
         }
    })";

    conf = conty_conf_from_string(conf_string.c_str());

    EXPECT_TRUE(conf == NULL);
}

TEST(conty_conf, net_bridge_ip_missing)
{
    struct conty_conf *conf = NULL;
    std::string conf_string = R"({
        "sandbox_id": "first-container-ever",
        "rootfs": "/some/directory",
        "net": {
            "veth_ip": "192.168.0.102",
         }
    })";

    conf = conty_conf_from_string(conf_string.c_str());

    EXPECT_TRUE(conf == NULL);
}

TEST(conty_conf, rootfs_missing)
{
    struct conty_conf *conf = NULL;
    std::string conf_string = R"({
        "sandbox_id": "first-container-ever",
        "net": {
            "veth_ip": "192.168.0.101",
            "bridge_ip": "192.168.0.102",
         }
    })";

    conf = conty_conf_from_string(conf_string.c_str());

    EXPECT_TRUE(conf == NULL);
}

TEST(conty_conf, sandbox_id_missing)
{
    struct conty_conf *conf = NULL;
    std::string conf_string = R"({
        "rootfs": "/some/directory",
        "net": {
            "veth_ip": "192.168.0.101",
            "bridge_ip": "192.168.0.102",
         }
    })";

    conf = conty_conf_from_string(conf_string.c_str());

    EXPECT_TRUE(conf == NULL);
}

TEST(conty_conf, sandbox_id_empty)
{
    struct conty_conf *conf = NULL;
    std::string conf_string = R"({
        "sandbox_id": "",
        "rootfs": "/some/directory",
        "net": {
            "veth_ip": "192.168.0.101",
            "bridge_ip": "192.168.0.102",
         }
    })";

    conf = conty_conf_from_string(conf_string.c_str());

    EXPECT_TRUE(conf == NULL);
}

TEST(conty_conf, rootfs_empty)
{
    struct conty_conf *conf = NULL;
    std::string conf_string = R"({
        "sandbox_id": "first-container-ever",
        "rootfs": "",
        "net": {
            "veth_ip": "192.168.0.101",
            "bridge_ip": "192.168.0.102",
         }
    })";

    conf = conty_conf_from_string(conf_string.c_str());

    EXPECT_TRUE(conf == NULL);
}

TEST(conty_conf, net_veth_ip_empty)
{
    struct conty_conf *conf = NULL;
    std::string conf_string = R"({
        "sandbox_id": "first-container-ever",
        "rootfs": "/some/directory",
        "net": {
            "veth_ip": "",
            "bridge_ip": "192.168.0.102",
         }
    })";

    conf = conty_conf_from_string(conf_string.c_str());

    EXPECT_TRUE(conf == NULL);
}

TEST(conty_conf, net_bridge_ip_empty)
{
    struct conty_conf *conf = NULL;
    std::string conf_string = R"({
        "sandbox_id": "first-container-ever",
        "rootfs": "/some/directory",
        "net": {
            "veth_ip": "192.168.0.101",
            "bridge_ip": "",
         }
    })";

    conf = conty_conf_from_string(conf_string.c_str());

    EXPECT_TRUE(conf == NULL);
}