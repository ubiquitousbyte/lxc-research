cmake_minimum_required(VERSION 3.14)

find_package(BpfGen REQUIRED)

add_bpf_skeleton(test test.bpf.c)
add_executable(test test.c)
target_link_libraries(test test_skel)