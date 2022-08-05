include(ExternalProject)
find_program(MAKE_BIN NAMES make)

ExternalProject_Add(libnl
        PREFIX libnl
        URL https://github.com/thom311/libnl/releases/download/libnl3_7_0/libnl-3.7.0.tar.gz
        URL_HASH SHA256=9fe43ccbeeea72c653bdcf8c93332583135cda46a79507bfd0a483bb57f65939

        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libnl
        BUILD_IN_SOURCE true

        CONFIGURE_COMMAND
            ./configure --prefix=<INSTALL_DIR> --sysconfdir=<INSTALL_DIR>

        BUILD_COMMAND ${MAKE_BIN}
        INSTALL_COMMAND ${MAKE_BIN} install
        )

ExternalProject_Get_Property(libnl INSTALL_DIR)

add_library(nl UNKNOWN IMPORTED)
set_target_properties(nl PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libnl-3.a)
add_dependencies(nl libnl)

add_library(rtnl UNKNOWN IMPORTED)
set_target_properties(rtnl PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libnl-route-3.a)
add_dependencies(rtnl libnl)

add_library(nfnl UNKNOWN IMPORTED)
set_target_properties(nfnl PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libnl-nf-3.a)
add_dependencies(nfnl libnl)

include_directories(${INSTALL_DIR}/include/libnl3 BEFORE)