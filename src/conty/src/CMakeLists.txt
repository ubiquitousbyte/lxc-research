cmake_minimum_required(VERSION 3.14)
project(conty-bench
        VERSION 1.0.0
        DESCRIPTION "Container benchmarking tool using eBPF tracing"
        LANGUAGES C)

set(ROOTFS_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/rootfs)

add_custom_command(
        OUTPUT
            ${ROOTFS_BASE_DIR}/net
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_LIST_DIR}
        COMMAND
            bash net-rootfs.sh ${ROOTFS_BASE_DIR} net)
add_custom_target(net-rootfs DEPENDS ${ROOTFS_BASE_DIR}/net)

add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/net-hook
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_LIST_DIR}/hooks/cmd/network
        COMMAND
            CGO_ENABLED=0 go build -o ${CMAKE_CURRENT_BINARY_DIR}/net-hook)
add_custom_target(net-hook DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/net-hook)

#add_subdirectory(bpf)

add_executable(conty-netbench net.c)
target_link_libraries(conty-netbench conty)
configure_file(${CMAKE_CURRENT_LIST_DIR}/net-config.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
add_dependencies(conty-netbench net-rootfs net-hook)


add_custom_command(
        OUTPUT
            ${ROOTFS_BASE_DIR}/io
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_LIST_DIR}
        COMMAND
            bash io-rootfs.sh ${ROOTFS_BASE_DIR} io)
add_custom_target(io-rootfs DEPENDS ${ROOTFS_BASE_DIR}/io)

add_executable(conty-iobench io.c)
target_link_libraries(conty-iobench conty)
configure_file(${CMAKE_CURRENT_LIST_DIR}/io-config.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
add_dependencies(conty-iobench io-rootfs)

add_executable(conty-runtime runtime.c hash.h runtime.h)
target_link_libraries(conty-runtime conty)
