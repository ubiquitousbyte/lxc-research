#include <gtest/gtest.h>

#include "hook.h"
#include <errno.h>

TEST(conty_hook, exec_timeout)
{
    int status;
    struct conty_hook hook;
    conty_hook_init(&hook, "/usr/bin/sleep");
    struct conty_hook_param param1 = {
            .param = "24"
    };
    conty_hook_put_arg(&hook, &param1);
    char buf[64];
    memset(buf, 0, sizeof buf);


    ASSERT_EQ(conty_hook_exec(&hook, buf, sizeof buf, &status), -ETIMEDOUT);
}

TEST(conty_hook, exec)
{
    int status;
    struct conty_hook hook;
    conty_hook_init(&hook, "/usr/bin/sleep");
    struct conty_hook_param param1 = {
            .param = "2"
    };
    conty_hook_put_arg(&hook, &param1);
    char buf[64];
    memset(buf, 0, sizeof buf);

    ASSERT_EQ(conty_hook_exec(&hook, buf, sizeof buf, &status), 0);
}