cmake_minimum_required(VERSION 3.14)
project(nsbench
        VERSION 1.0.0
        DESCRIPTION "Benchmark CLI for measuring system performance"
        LANGUAGES C)

add_executable(nsbench
        nsbench.c)
target_link_libraries(nsbench conty)