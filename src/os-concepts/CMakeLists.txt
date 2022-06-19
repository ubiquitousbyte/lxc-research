cmake_minimum_required(VERSION 3.14)
project(os-concepts
        LANGUAGES C
        VERSION 0.1
        DESCRIPTION "Operating system concepts")

set(CMAKE_C_STANDARD 99)

include(deps/libcap.cmake)

add_subdirectory(src)