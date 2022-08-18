cmake_minimum_required(VERSION 3.14)

set(NSBENCH_NET_WORKLOAD ${CMAKE_CURRENT_BINARY_DIR}/net-workload)

add_custom_command(
    OUTPUT ${NSBENCH_NET_WORKLOAD}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND go build -o ${NSBENCH_NET_WORKLOAD})

add_custom_target(net-workload DEPENDS ${NSBENCH_NET_WORKLOAD})