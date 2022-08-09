#include <gtest/gtest.h>

#include <conty/conty.h>

TEST(conty_hook, exec_times_out)
{
    const char *prog = "/usr/bin/sleep";
    const char *argv[] = { "/usr/bin/sleep", "20", (char *) NULL };
    const char *envp[] = { NULL };
    const char buf[1] = { 0 };
    int error;

    error = conty_hook_exec(prog, argv, envp, buf, 1, 2);

    EXPECT_EQ(error, -1);
    EXPECT_EQ(errno, ETIME);
}

TEST(conty_hook, exec)
{
    const char *prog = "/usr/bin/sleep";
    const char *argv[] = { "/usr/bin/sleep", "2", (char *) NULL };
    const char *envp[] = { NULL };
    const char buf[1] = { 0 };
    int error;

    error = conty_hook_exec(prog, argv, envp, buf, 1, 4);

    EXPECT_EQ(error, 0);
}