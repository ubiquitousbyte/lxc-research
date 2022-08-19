cmake_minimum_required(VERSION 3.14)

set(NSBENCH_DISK_WORKLOAD ${CMAKE_CURRENT_BINARY_DIR}/nsbench-disk-workload)

add_custom_command(
        OUTPUT ${NSBENCH_DISK_WORKLOAD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND go build -o ${NSBENCH_DISK_WORKLOAD})
add_custom_target(nsbench-disk-workload DEPENDS ${NSBENCH_DISK_WORKLOAD})
add_dependencies(nsbench-disk-workload nsbench-rootfs-setup)

set(NSBENCH_DISK_WORKLOAD_ROOTFS ${NSBENCH_ROOTFS_DIR}/disk/nsbench-disk-workload)
add_custom_command(
        OUTPUT ${NSBENCH_DISK_WORKLOAD_ROOTFS}
        COMMAND
            ${CMAKE_COMMAND} -E copy ${NSBENCH_DISK_WORKLOAD} ${NSBENCH_DISK_WORKLOAD_ROOTFS})
add_custom_target(nsbench-copy-disk-workload DEPENDS ${NSBENCH_DISK_WORKLOAD_ROOTFS})
configure_file(${CMAKE_CURRENT_LIST_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
add_dependencies(nsbench-copy-disk-workload nsbench-disk-workload)