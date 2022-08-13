#include "oci.h"

#include "log.h"

int test_hook_exec_timeout()
{
    char *argv[3] = { "/usr/bin/sleep", "5", (char *) NULL};
    struct oci_hook hook = {
            .ohk_path = "/usr/bin/sleep",
            .ohk_argv = argv,
            .ohk_envp = NULL,
            .ohk_timeout = 2
    };

    struct oci_process_state state = {
            .opst_container_id = "some-container",
            .opst_pid = 50,
            .opst_rootfs = "/path/to/bundle",
            .opst_status = "created"
    };

    if (oci_hook_exec(&hook, &state) >= 0)
        return -1;

    return 0;
}

int test_hook_exec()
{
    char *argv[3] = { "/usr/bin/sleep", "3", (char *) NULL};
    struct oci_hook hook = {
            .ohk_path = "/usr/bin/sleep",
            .ohk_argv = argv,
            .ohk_envp = NULL,
            .ohk_timeout = 5
    };

    struct oci_process_state state = {
            .opst_container_id = "some-container",
            .opst_pid = 50,
            .opst_rootfs = "/path/to/bundle",
            .opst_status = "created"
    };

    if (oci_hook_exec(&hook, &state) != 0)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    if (test_hook_exec() != 0) {
        LOG_ERROR("test_hook_exec failed");
        return EXIT_FAILURE;
    }

    if (test_hook_exec_timeout() != 0) {
        LOG_ERROR("test_hook_exec_timeout failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}