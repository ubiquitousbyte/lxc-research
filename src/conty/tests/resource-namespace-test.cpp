#include <gtest/gtest.h>

#include <conty/conty.h>

TEST(Namespace, from_proc_no_such_process)
{
    pid_t pid = -1;
    EXPECT_THROW(resource_namespace::from_proc(pid,resource_namespace::type::Net),
                 std::system_error);
}

TEST(Namespace, from_proc_current)
{
    pid_t pid = getpid();

    auto ns = resource_namespace::from_proc(pid,resource_namespace::type::Net);

    EXPECT_EQ(ns.namespace_type(), resource_namespace::type::Net);
    EXPECT_GT(ns.inode(), 0);
}