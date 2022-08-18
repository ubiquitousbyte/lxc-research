cmake_minimum_required(VERSION 3.14)

set(NSBENCH_NET_WORKLOAD ${CMAKE_CURRENT_BINARY_DIR}/nsbench-net-workload)

add_custom_command(
    OUTPUT ${NSBENCH_NET_WORKLOAD}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND go build -o ${NSBENCH_NET_WORKLOAD})

add_custom_target(nsbench-net-workload DEPENDS ${NSBENCH_NET_WORKLOAD})
add_dependencies(nsbench-net-workload nsbench-rootfs-setup)

set(NSBENCH_NET_WORKLOAD_ROOTFS ${NSBENCH_ROOTFS_DIR}/net/nsbench-net-workload)
add_custom_command(
        OUTPUT ${NSBENCH_NET_WORKLOAD_ROOTFS}
        COMMAND
            ${CMAKE_COMMAND} -E copy ${NSBENCH_NET_WORKLOAD} ${NSBENCH_NET_WORKLOAD_ROOTFS})
add_custom_target(nsbench-copy-net-workload DEPENDS ${NSBENCH_NET_WORKLOAD_ROOTFS})
configure_file(${CMAKE_CURRENT_LIST_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
add_dependencies(nsbench-copy-net-workload nsbench-net-workload)