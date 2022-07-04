cmake_minimum_required(VERSION 3.14)
project(conty
        LANGUAGES C CXX
        VERSION 0.1
        DESCRIPTION "Profiling-aware container runtime")

include(deps/libbpf.cmake)

add_executable(elf-test main.c)
target_link_libraries(elf-test PUBLIC bpf elf zlib)