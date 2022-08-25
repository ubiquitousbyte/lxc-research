cmake_minimum_required(VERSION 3.14)
project(conty-runtime
        VERSION 1.0.0
        DESCRIPTION "Basic container runtime"
        LANGUAGES C)

#add_subdirectory(workloads)
#add_subdirectory(hooks)

add_subdirectory(bpf)

add_executable(conty-runtime runtime.c runtime.h)
target_link_libraries(conty-runtime PUBLIC conty)