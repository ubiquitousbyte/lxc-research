#include <gtest/gtest.h>

#include "namespace.h"

#include <unistd.h>
#include <errno.h>

TEST(conty_ns, open_invalid_id)
{
    struct conty_ns *ns = conty_ns_open(0, CONTY_NS_UTS);
    EXPECT_EQ(errno, ESRCH);
    EXPECT_TRUE(ns == NULL);
}

TEST(conty_ns, open)
{
    struct conty_ns *ns = conty_ns_open(getpid(), CONTY_NS_UTS);
    EXPECT_TRUE(ns != NULL);

    ino_t inode = conty_ns_inode(ns);
    dev_t device = conty_ns_device(ns);

    EXPECT_NE(inode, 0);
    EXPECT_NE(device, 0);
}

TEST(conty_ns, is)
{
    struct conty_ns *left = conty_ns_open_current(CONTY_NS_UTS);
    struct conty_ns *right = conty_ns_open(getpid(), CONTY_NS_UTS);
    EXPECT_TRUE(conty_ns_is(left, right));
}

TEST(conty_ns, detach_and_fork_join)
{
    /*
     * The parent detaches itself into a new execution context consisting
     * of a user and uts namespace
     */
    int rc = conty_ns_detach(CLONE_NEWUSER | CLONE_NEWUTS);
    EXPECT_EQ(rc, 0);

    /* Get an in-memory handle on the newly-created uts namespace */
    struct conty_ns *ns = conty_ns_open_current(CONTY_NS_UTS);

    /* Spawn a child */
    pid_t child = fork();
    EXPECT_NE(child, -1);
    if (child == 0) {
        /* The child has duped the parent's namespace handle, so it can join */
        rc = conty_ns_join(ns);

        /* The child gets an in-memory handle to its uts namespace */
        struct conty_ns *child_ns = conty_ns_open_current(CONTY_NS_UTS);

        EXPECT_EQ(rc, 0);
        /* Make sure the child namespace and parent namespace are the same */
        EXPECT_TRUE(conty_ns_is(child_ns, ns));
        exit(0);
    }

    /* Wait for child */
    EXPECT_EQ(waitpid(child, NULL, 0), child);
}