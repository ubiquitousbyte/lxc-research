#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <conty/oci.h>

using json = nlohmann::json;

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
}
