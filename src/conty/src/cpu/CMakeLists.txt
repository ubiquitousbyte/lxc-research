cmake_minimum_required(VERSION 3.14)

set(NSBENCH_CPU_WORKLOAD ${CMAKE_CURRENT_BINARY_DIR}/cpu-workload)

add_custom_command(
        OUTPUT ${NSBENCH_CPU_WORKLOAD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND go build -o ${NSBENCH_CPU_WORKLOAD})

add_custom_target(cpu-workload DEPENDS ${NSBENCH_CPU_WORKLOAD})