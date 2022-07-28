#include <gtest/gtest.h>
#include <conty/conty.h>

TEST(conty_hook, conty_hook_exec_timeout)
{
    struct conty_hook hook;
    conty_hook_init(&hook, "/usr/bin/ls");

    struct conty_hook_param param = {
            .param = "/usr/bin/sleep",
    };
    conty_hook_put_arg(&hook, &param);

    struct conty_hook_param param1 = {
            .param = "50"
    };

    conty_hook_put_arg(&hook, &param1);

    char buf[64];
    memset(buf, 0, sizeof buf);
    conty_hook_exec(&hook, buf, sizeof buf);
}