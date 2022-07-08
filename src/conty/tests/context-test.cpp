#include <gtest/gtest.h>

#include <conty/conty.h>

TEST(Context, find)
{
    pid_t pid = -1;
    EXPECT_THROW(context::find(pid,context::variant::Net),
                 std::system_error);
}

TEST(Context, current)
{
    auto ns = context::current(context::variant::Net);
    EXPECT_EQ(ns.type(), context::variant::Net);
}