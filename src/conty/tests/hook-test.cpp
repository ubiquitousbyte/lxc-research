#include <gtest/gtest.h>

#include "hook.h"

TEST(conty_hook, exec_times_out)
{
    const char *prog = "/usr/bin/sleep";
    const char *argv[] = { "/usr/bin/sleep", "20", (char *) NULL };
    const char *envp[] = { NULL };
    struct conty_hook hook = {
            .prog    = prog,
            .argv    = argv,
            .envp    = envp,
            .timeout = 2000
    };
    const char buf[1] = { 0 };
    int error;

    error = conty_hook_exec(&hook, buf, sizeof(buf));

    EXPECT_EQ(error, -1);
    EXPECT_EQ(errno, ETIME);
}

TEST(conty_hook, exec)
{
    const char *prog = "/usr/bin/sleep";
    const char *argv[] = { "/usr/bin/sleep", "2", (char *) NULL };
    const char *envp[] = { NULL };
    struct conty_hook hook = {
            .prog    = prog,
            .argv    = argv,
            .envp    = envp,
            .timeout = 5000
    };
    const char buf[1] = { 0 };
    int error;

    error = conty_hook_exec(&hook, buf, sizeof(buf));

    EXPECT_EQ(error, 0);
}