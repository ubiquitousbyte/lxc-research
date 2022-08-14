cmake_minimum_required(VERSION 3.14)
project(conty 
    VERSION 1.0.0
    DESCRIPTION "Basic container runtime for Linux"
    LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
string(APPEND CMAKE_C_FLAGS "-D_GNU_SOURCE")
#add_compile_definitions($<$<CONFIG:DEBUG>:CONTY_DEBUG>)

# Dependencies
include(dependencies/libjsonc.cmake)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)