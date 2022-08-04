cmake_minimum_required(VERSION 3.14)
project(conty 
    VERSION 1.0.0
    DESCRIPTION "Basic container runtime for Linux"
    LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
string(APPEND CMAKE_C_FLAGS "-D_GNU_SOURCE")
add_compile_definitions($<$<CONFIG:DEBUG>:CONTY_DEBUG>)

# Dependencies
include(dependencies/libcap.cmake)

set(CONTY_PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
set(CONTY_PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
set(CONTY_PUBLIC_HDR ${CONTY_PUBLIC}/conty)
message(${CONTY_PUBLIC})
add_library(conty STATIC)
target_sources(conty 
    PRIVATE
        ${CONTY_PRIVATE}/namespace.c
        ${CONTY_PRIVATE}/namespace.h
        ${CONTY_PRIVATE}/hook.c
        ${CONTY_PRIVATE}/hook.h
        ${CONTY_PRIVATE}/mount.h
        ${CONTY_PRIVATE}/mount.c
        ${CONTY_PRIVATE}/syscall.h
        ${CONTY_PRIVATE}/queue.h
        ${CONTY_PRIVATE}/sandbox.h
        ${CONTY_PRIVATE}/sandbox.c
        ${CONTY_PRIVATE}/user.h
        ${CONTY_PRIVATE}/user.c
        ${CONTY_PRIVATE}/resource.h
        ${CONTY_PRIVATE}/clone.h
        ${CONTY_PRIVATE}/clone.c
        ${CONTY_PRIVATE}/ipc.h
        ${CONTY_PRIVATE}/ipc.c
    PUBLIC
        ${CONTY_PUBLIC_HDR}/conty.h)

target_include_directories(conty
        PUBLIC
            ${CONTY_PUBLIC}
            "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:TEST>>:${CONTY_PRIVATE}>>"
        )
target_link_libraries(conty PRIVATE cap)
add_subdirectory(tests)
