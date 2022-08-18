cmake_minimum_required(VERSION 3.14)
project(nsbench
        VERSION 1.0.0
        DESCRIPTION "Benchmark CLI for measuring system performance"
        LANGUAGES C)

add_subdirectory(cpu)
add_subdirectory(disk)
add_subdirectory(net)

set(NSBENCH_ROOTFS_DIR ${CMAKE_CURRENT_BINARY_DIR}/rootfs)

add_custom_command(
        OUTPUT ${NSBENCH_ROOTFS_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND bash rootfs.sh ${NSBENCH_ROOTFS_DIR})

add_custom_target(nsbench-rootfs-setup DEPENDS ${NSBENCH_ROOTFS_DIR})

add_executable(nsbench nsbench.c)
target_link_libraries(nsbench conty)
add_dependencies(nsbench
        nsbench-rootfs-setup
        net-workload
        cpu-workload
        disk-workload
        net-workload)