cmake_minimum_required(VERSION 3.14)

set(NSBENCH_CPU_WORKLOAD ${CMAKE_CURRENT_BINARY_DIR}/nsbench-cpu-workload)

add_custom_command(
        OUTPUT ${NSBENCH_CPU_WORKLOAD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND go build -o ${NSBENCH_CPU_WORKLOAD})
add_custom_target(nsbench-cpu-workload DEPENDS ${NSBENCH_CPU_WORKLOAD})
add_dependencies(nsbench-cpu-workload nsbench-rootfs-setup)

set(NSBENCH_CPU_WORKLOAD_ROOTFS ${NSBENCH_ROOTFS_DIR}/cpu/nsbench-cpu-workload)
add_custom_command(
        OUTPUT ${NSBENCH_CPU_WORKLOAD_ROOTFS}
        COMMAND
            ${CMAKE_COMMAND} -E copy ${NSBENCH_CPU_WORKLOAD} ${NSBENCH_CPU_WORKLOAD_ROOTFS})
add_custom_target(nsbench-copy-cpu-workload DEPENDS ${NSBENCH_CPU_WORKLOAD_ROOTFS})
configure_file(${CMAKE_CURRENT_LIST_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
add_dependencies(nsbench-copy-cpu-workload nsbench-cpu-workload)