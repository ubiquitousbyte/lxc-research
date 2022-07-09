cmake_minimum_required(VERSION 3.14)
project(conty 
    VERSION 1.0.0
    DESCRIPTION "Basic container runtime for Linux"
    LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)

set(CONTY_PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
set(CONTY_PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
set(CONTY_PUBLIC_HDR ${CONTY_PUBLIC}/conty)

add_library(conty STATIC)
target_sources(conty 
    PRIVATE
        ${CONTY_PRIVATE}/namespace.c
        ${CONTY_PRIVATE}/namespace.h
    PUBLIC
        ${CONTY_PUBLIC_HDR}/conty.h)
target_include_directories(conty PUBLIC ${CONTY_PUBLIC})
target_include_directories(conty PUBLIC "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:TEST>>:${CONTY_PRIVATE}>>")

add_subdirectory(tests)
