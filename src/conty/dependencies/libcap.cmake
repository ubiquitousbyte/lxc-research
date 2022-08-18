include(ExternalProject)
find_program(MAKE_BIN NAMES make)

ExternalProject_Add(libcap
        PREFIX libcap
        URL https://git.kernel.org/pub/scm/libs/libcap/libcap.git/snapshot/libcap-2.63.tar.gz
        URL_HASH SHA256=64cc82775bad5b2dd9f8f4894366ff233c809ebb551fcff67b7ca21f4cb37863

        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libcap
        BUILD_IN_SOURCE true

        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${MAKE_BIN} FAKEROOT=<INSTALL_DIR>
        INSTALL_COMMAND ${MAKE_BIN} FAKEROOT=<INSTALL_DIR> install
        )

ExternalProject_Get_Property(libcap INSTALL_DIR)

add_library(cap UNKNOWN IMPORTED GLOBAL)
set_target_properties(cap PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib64/libcap.a)
add_dependencies(cap libcap)
include_directories(${INSTALL_DIR}/usr/include BEFORE)