#include <gtest/gtest.h>

#include <unistd.h>

#include <net/if.h>

#include "network.h"
#include "resource.h"

TEST(conty_network, create)
{
    CONTY_INVOKE_CLEANER(conty_network_destroy) struct conty_network *net = NULL;

    net = conty_network_create("br0", "veth0", "veth1", getpid());

    EXPECT_TRUE(net != NULL);
    EXPECT_GT(if_nametoindex("br0"), 0);
    EXPECT_GT(if_nametoindex("veth0"), 0);
    EXPECT_GT(if_nametoindex("veth1"), 0);
}
