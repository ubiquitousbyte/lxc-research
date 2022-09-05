cmake_minimum_required(VERSION 3.14)
project(conty-runtime
        VERSION 1.0.0
        DESCRIPTION "Container runtime server"
        LANGUAGES C)

add_executable(conty-runtime runtime.c hash.h runtime.h)
target_link_libraries(conty-runtime conty)
target_include_directories(conty-runtime
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)