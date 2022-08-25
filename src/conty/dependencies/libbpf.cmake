

cmake_minimum_required(VERSION 3.14)

include(ExternalProject)

find_program(MAKE NAMES make)

ExternalProject_Add(libbpf
        PREFIX libbpf
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../libbpf/src

        CONFIGURE_COMMAND ""

        BUILD_COMMAND
        ${MAKE}
        BUILD_STATIC_ONLY=1
        OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf
        DESTDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf
        INCLUDEDIR=
        LIBDIR=
        UAPIDIR=
        install
        BUILD_IN_SOURCE TRUE

        INSTALL_COMMAND ""

        STEP_TARGETS build)

set(LIBBPF_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/libbpf)
set(LIBBPF_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf.a)