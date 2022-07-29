#include <gtest/gtest.h>

#include <conty/conty.h>
#include <errno.h>

#define CONTY_HOOK_EXECVE_ARGS(hook_args, args, len) do { \
    __typeof(len) ___len = len;                            \
    args[___len] = NULL;                                        \
    struct conty_hook_param *item, *tmp;             \
    SLIST_FOREACH_SAFE(item, hook_args, next, tmp)        \
        argv[--___len] = item->param;                   \
} while(0)


TEST(conty_hook, conty_hook_exec_timeout)
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

TEST(conty_hook, conty_hook_exec)
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