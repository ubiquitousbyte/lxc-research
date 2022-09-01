cmake_minimum_required(VERSION 3.14)

find_package(BpfGen REQUIRED)

add_bpf_skeleton(tcplatency tcplatency.bpf.c)
add_bpf_skeleton(rqlatency rqlatency.bpf.c)
add_bpf_skeleton(vfslatency vfslatency.bpf.c)

add_library(contybpf STATIC)
target_sources(contybpf
    PRIVATE
        histogram.h
        fs.h
        trace.c
    PUBLIC
        ../../include/conty/bpf.h)

target_include_directories(contybpf
        PUBLIC
            ../../include)

target_link_libraries(contybpf
        tcplatency_skel
        rqlatency_skel
        vfslatency_skel)
