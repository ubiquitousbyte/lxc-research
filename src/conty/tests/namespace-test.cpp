#include <gtest/gtest.h>

#include "namespace.h"

#include <unistd.h>
#include <errno.h>

TEST(conty_ns, open_invalid_id)
{
    struct conty_ns *ns = conty_ns_open(0, CLONE_NEWUTS);
    EXPECT_EQ(errno, ESRCH);
    EXPECT_TRUE(ns == NULL);
}

TEST(conty_ns, open)
{
    struct conty_ns *ns = conty_ns_open(getpid(), CLONE_NEWUTS);
    EXPECT_TRUE(ns != NULL);

    ino_t inode = conty_ns_inode(ns);
    dev_t device = conty_ns_device(ns);

    EXPECT_NE(inode, 0);
    EXPECT_NE(device, 0);
}

TEST(conty_ns, is)
{
    struct conty_ns *left = conty_ns_open_current(CLONE_NEWUTS);
    struct conty_ns *right = conty_ns_open(getpid(), CLONE_NEWUTS);
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
    struct conty_ns *ns = conty_ns_open_current(CLONE_NEWUTS);

    /* Spawn a child */
    pid_t child = fork();
    EXPECT_NE(child, -1);
    if (child == 0) {
        /* The child has duped the parent's namespace handle, so it can join */
        rc = conty_ns_join(ns);

        /* The child gets an in-memory handle to its uts namespace */
        struct conty_ns *child_ns = conty_ns_open_current(CLONE_NEWUTS);

        EXPECT_EQ(rc, 0);
        /* Make sure the child namespace and parent namespace are the same */
        EXPECT_TRUE(conty_ns_is(child_ns, ns));
        exit(0);
    }

    /* Wait for child */
    EXPECT_EQ(waitpid(child, NULL, 0), child);
}

TEST(conty_ns, parent_nonhierarchical_ns)
{
    struct conty_ns *ns = conty_ns_open_current(CLONE_NEWUTS);
    EXPECT_TRUE(ns != NULL);
    EXPECT_TRUE(conty_ns_parent(ns) == NULL);
    EXPECT_EQ(errno, EINVAL);
}

TEST(conty_ns, parent)
{
    /* Get a handle on the current pid namespace */
    struct conty_ns *init_pid_ns = conty_ns_open_current(CLONE_NEWPID);
    EXPECT_TRUE(init_pid_ns != NULL);

    /*
     * Detach the current process into a new pid namespace
     * Note that the parent is still in the original namespace.
     * Otherwise, its pid will have to change which is a no go
     * Instead, all children forked from this process will reside in the
     * newly allocated namespace
     */
    int rc = conty_ns_detach(CLONE_NEWPID);
    EXPECT_EQ(rc, 0);

    /* Fork a child that will be put in the new pid namespace */
    pid_t child = fork();
    EXPECT_NE(child, -1);

    if (child == 0) {
        /* Get a handle on the child pid namespace */
        struct conty_ns *child_pid_ns = conty_ns_open_current(CLONE_NEWPID);
        EXPECT_FALSE(conty_ns_is(init_pid_ns, child_pid_ns));

        /* Get a handle on the parent pid namespace of this child */
        struct conty_ns *child_parent_pid_ns = conty_ns_parent(child_pid_ns);
        EXPECT_TRUE(conty_ns_is(child_parent_pid_ns, init_pid_ns));
    }

    /* Wait for child */
    EXPECT_EQ(waitpid(child, NULL, 0), child);
}

TEST(conty_ns_id_map, init)
{
    char buf[CONTY_NS_ID_MAP_MAX];
    struct conty_ns_id_map map;
    int rc;

    rc = conty_ns_id_map_init(&map, buf, sizeof(buf));

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(map.cap, CONTY_NS_ID_MAP_MAX);
    EXPECT_EQ(map.written, 0);

    char buf2[CONTY_NS_ID_MAP_MAX*2];
    rc = conty_ns_id_map_init(&map, buf2, sizeof(buf2));

    EXPECT_EQ(rc, -ENOSPC);
}

TEST(conty_ns_id_map, put)
{
    char buf[CONTY_NS_ID_MAP_MAX];
    struct conty_ns_id_map map;
    int rc;
    EXPECT_EQ(conty_ns_id_map_init(&map, buf, sizeof(buf)), 0);

    rc = conty_ns_id_map_put(&map, 0, 100, 5);

    EXPECT_EQ(rc, 0);

    EXPECT_EQ(map.written, strlen("0 100 5\n"));
}