cmake_minimum_required(VERSION 3.14)

set(NSBENCH_ROOTFS_DIR ${CMAKE_CURRENT_BINARY_DIR}/rootfs)
add_custom_command(
        OUTPUT ${NSBENCH_ROOTFS_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND bash rootfs.sh ${NSBENCH_ROOTFS_DIR})
add_custom_target(nsbench-rootfs-setup DEPENDS ${NSBENCH_ROOTFS_DIR})

add_subdirectory(cpu)
add_subdirectory(disk)
add_subdirectory(net)
