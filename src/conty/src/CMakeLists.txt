cmake_minimum_required(VERSION 3.14)
project(conty-bench
        VERSION 1.0.0
        DESCRIPTION "Container benchmarking tool using eBPF tracing"
        LANGUAGES C)

find_package(BpfGen REQUIRED)

add_bpf_skeleton(tcplatency tcplatency.bpf.c)
add_bpf_skeleton(rqlatency rqlatency.bpf.c)
add_bpf_skeleton(vfslatency vfslatency.bpf.c)

add_executable(conty-bench bench.c bench.h histogram.h tcplatency.h tcplatency.c resource.h log.h log.c tick.h tick.c rqlatency.h rqlatency.c vfslatency.c vfslatency.h histogram.c)
target_link_libraries(conty-bench tcplatency_skel rqlatency_skel vfslatency_skel)
