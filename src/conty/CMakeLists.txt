cmake_minimum_required(VERSION 3.14)
project(conty 
    VERSION 1.0.0
    DESCRIPTION "Basic container runtime for Linux"
    LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
include(FetchContent)

FetchContent_Declare(json 
    URL 
        https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz)
FetchContent_MakeAvailable(json)

set(CONTY_PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
set(CONTY_PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
set(CONTY_PUBLIC_HDR ${CONTY_PUBLIC}/conty)

add_library(conty STATIC)
target_sources(conty 
    PRIVATE
        ${CONTY_PRIVATE}/oci.cpp
        ${CONTY_PRIVATE}/context.cpp
    PUBLIC
        ${CONTY_PUBLIC_HDR}/oci.h
        ${CONTY_PUBLIC_HDR}/conty.h)
target_include_directories(conty PUBLIC ${CONTY_PUBLIC})
target_link_libraries(conty PRIVATE nlohmann_json::nlohmann_json)

add_subdirectory(tests)
