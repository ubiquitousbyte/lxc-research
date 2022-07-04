include(ExternalProject)
find_program(__MAKE NAMES make)

include(deps/libelf.cmake)

ExternalProject_Add(libbpf
        PREFIX libbpf
        URL https://github.com/libbpf/libbpf/archive/refs/tags/v0.8.0.tar.gz

        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libbpf

        CONFIGURE_COMMAND ""
        BUILD_COMMAND
            PKG_CONFIG_PATH=${CMAKE_CURRENT_BINARY_DIR}/libz/lib/pkgconfig:${CMAKE_CURRENT_BINARY_DIR}/libelf/lib/pkgconfig PREFIX=<INSTALL_DIR> LIBDIR=<INSTALL_DIR>/lib ${__MAKE} -C <SOURCE_DIR>/src
        INSTALL_COMMAND
            PKG_CONFIG_PATH=${CMAKE_CURRENT_BINARY_DIR}/libz/lib/pkgconfig:${CMAKE_CURRENT_BINARY_DIR}/libelf/lib/pkgconfig PREFIX=<INSTALL_DIR> LIBDIR=<INSTALL_DIR>/lib ${__MAKE} -C <SOURCE_DIR>/src install

        DEPENDS libz libelf
        )

ExternalProject_Get_Property(libbpf INSTALL_DIR)

add_library(bpf UNKNOWN IMPORTED)
set_target_properties(bpf
        PROPERTIES
            IMPORTED_LOCATION ${INSTALL_DIR}/lib/libbpf.a
        )
add_dependencies(bpf libbpf)
include_directories(BEFORE ${INSTALL_DIR}/include)