cmake_minimum_required(VERSION 3.14)

include(ExternalProject)

find_program(MAKE NAMES make)

ExternalProject_Add(bpftool
        PREFIX bpftool
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../bpftool/src

        CONFIGURE_COMMAND ""

        BUILD_COMMAND
        ${MAKE}
        OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/bpftool/
        BUILD_IN_SOURCE TRUE

        INSTALL_COMMAND ""

        STEP_TARGETS build)

set(BPFGEN_BPFTOOL ${CMAKE_CURRENT_BINARY_DIR}/bpftool/bpftool)