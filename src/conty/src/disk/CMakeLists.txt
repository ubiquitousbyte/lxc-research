cmake_minimum_required(VERSION 3.14)

set(NSBENCH_DISK_WORKLOAD ${CMAKE_CURRENT_BINARY_DIR}/disk-workload)

add_custom_command(
        OUTPUT ${NSBENCH_DISK_WORKLOAD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND go build -o ${NSBENCH_DISK_WORKLOAD})

add_custom_target(disk-workload DEPENDS ${NSBENCH_DISK_WORKLOAD})