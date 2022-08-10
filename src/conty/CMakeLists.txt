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
include(dependencies/libcap.cmake)
include(dependencies/libjsonc.cmake)
include(dependencies/libnl.cmake)

set(CONTY_PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
set(CONTY_PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
set(CONTY_PUBLIC_HDR ${CONTY_PUBLIC}/conty)

add_library(conty STATIC)
target_sources(conty 
    PRIVATE
        ${CONTY_PRIVATE}/namespace.c
        ${CONTY_PRIVATE}/namespace.h
        ${CONTY_PRIVATE}/hook.c
        ${CONTY_PRIVATE}/mount.h
        ${CONTY_PRIVATE}/mount.c
        ${CONTY_PRIVATE}/syscall.h
        ${CONTY_PRIVATE}/user.h
        ${CONTY_PRIVATE}/user.c
        ${CONTY_PRIVATE}/resource.h
        ${CONTY_PRIVATE}/clone.h
        ${CONTY_PRIVATE}/clone.c
        ${CONTY_PRIVATE}/log.h
        ${CONTY_PRIVATE}/log.c
        ${CONTY_PRIVATE}/oci-json.c
        ${CONTY_PRIVATE}/container.h
        ${CONTY_PRIVATE}/container.c
        ${CONTY_PRIVATE}/sync.h
        ${CONTY_PRIVATE}/sync.c
    PUBLIC
        ${CONTY_PUBLIC_HDR}/conty.h
        ${CONTY_PUBLIC_HDR}/queue.h)

target_include_directories(conty
        PUBLIC
            ${CONTY_PUBLIC}
            "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:TEST>>:${CONTY_PRIVATE}>>"
        )

target_link_libraries(conty PRIVATE cap json-c nfnl rtnl nl)

add_subdirectory(tests)
