#include <gtest/gtest.h>

#include <errno.h>

#include "namespace.h"
#include "user.h"

TEST(conty_user, id_map_put_no_space)
{
    int err;
    char buf[4];
    struct conty_user_id_map map = {
            .buf = buf,
            .cap = sizeof(buf),
            .written = 0
    };

    err = conty_user_id_map_put(&map, 0, 1000, 1);

    EXPECT_EQ(err, -ENOSPC);
}

TEST(conty_user, id_map_put_success)
{
    int err;
    char buf[CONTY_USER_ID_MAP_MAX];
    struct conty_user_id_map map = {
            .buf = buf,
            .cap = sizeof(buf),
            .written = 0
    };

    err = conty_user_id_map_put(&map, 0, 100, 1);

    EXPECT_EQ(err, 0);
}

TEST(conty_user, write_own_uid_mappings)
{
    int err;
    char buf[CONTY_USER_ID_MAP_MAX];
    struct conty_user_id_map map;

    conty_user_id_map_init(&map, buf, sizeof(buf));
    conty_user_id_map_put(&map, 0, geteuid(), 1);
    conty_ns_unshare(CLONE_NEWUSER);

    err = conty_user_write_own_uid_mappings(&map);

    EXPECT_EQ(err, 0);
    EXPECT_EQ(seteuid(0), 0);
}
