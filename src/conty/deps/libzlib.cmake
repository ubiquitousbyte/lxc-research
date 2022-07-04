include(ExternalProject)
find_program(__MAKE NAMES make)

ExternalProject_Add(libz
        PREFIX libz
        URL https://zlib.net/zlib-1.2.12.tar.gz
        URL_HASH SHA256=91844808532e5ce316b3c010929493c0244f3d37593afd6de04f71821d5136d9

        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libz

        CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
        BUILD_COMMAND ${__MAKE}

        TEST_COMMAND ${__MAKE} check
        TEST_BEFORE_INSTALL true

        INSTALL_COMMAND ${__MAKE} install
        )

ExternalProject_Get_Property(libz INSTALL_DIR)

add_library(zlib UNKNOWN IMPORTED)
set_target_properties(zlib
        PROPERTIES
            IMPORTED_LOCATION ${INSTALL_DIR}/lib/libz.a
        )
add_dependencies(zlib libz)
include_directories(BEFORE ${INSTALL_DIR}/include)