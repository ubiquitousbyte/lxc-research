#include <gtest/gtest.h>

#include <conty/oci.h>

TEST(OCI, from_json)
{
    auto j = R"(
        {
            "ociVersion": "1.0.1",
            "hooks": {
                "prestart": [
                    {
                        "path": "/usr/bin/fix-mounts",
                        "args": [
                            "fix-mounts",
                                "arg1",
                                "arg2"
                            ],
                        "env": [
                            "key1=value1"
                        ]
                    },
                    {
                        "path": "/usr/bin/setup-network"
                    }
                ],
                "poststart": [
                    {
                        "path": "/usr/bin/notify-start",
                        "timeout": 5
                    }
                ],
                "poststop": [
                    {
                        "path": "/usr/sbin/cleanup.sh",
                        "args": [
                           "cleanup.sh",
                            "-f"
                        ]
                    }
                ]
            }
        }
    )";

    auto spec = oci::specification::from_json(j);

    EXPECT_STREQ("1.0.1", spec.version.c_str());
    EXPECT_EQ(spec.hooks.on_create_rt_depr.size(), 2);
    EXPECT_EQ(spec.hooks.on_create_rt.size(), 0);
    EXPECT_EQ(spec.hooks.on_start_cont.size(), 0);
    EXPECT_EQ(spec.hooks.on_running_cont.size(), 1);
    EXPECT_EQ(spec.hooks.on_stopped_cont.size(), 1);
}
