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
    EXPECT_GT(ns.inode(), 0);
}

TEST(Context, detach)
{
    char hostname[512];

    pid_t child = fork();
    if (child == 0) {
        context::detach(context::variant::User | context::variant::Uts);
        EXPECT_EQ(sethostname("conty", sizeof("conty")), 0);
        exit(0);
    }

    EXPECT_EQ(waitpid(child, nullptr, 0), child);
    EXPECT_EQ(gethostname(hostname, sizeof(hostname)), 0);
    EXPECT_STRNE(hostname, "conty");
}

TEST(Context, detach_perm)
{
    char hostname[512];

    pid_t child = fork();
    if (child == 0) {
        /*
         * Detach throws an error because the process cannot create a new
         * UTS namespace without having the CAP_SYS_ADMIN capability set.
         * If we were to set that, or add the context::variant::User flag,
         * then it would work, because the user will have that capability
         * in the namespace.
         */
        EXPECT_THROW(context::detach(context::variant::Uts), std::system_error);
        exit(0);
    }

    EXPECT_EQ(waitpid(child, nullptr, 0), child);
}
