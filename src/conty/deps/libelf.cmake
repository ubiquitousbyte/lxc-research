include(ExternalProject)
find_program(__MAKE NAMES make)

include(deps/libzlib.cmake)

ExternalProject_Add(libelf
        PREFIX libelf
        URL https://sourceware.org/elfutils/ftp/0.187/elfutils-0.187.tar.bz2
        URL_HASH SHA512=a9b9e32b503b8b50a62d4e4001097ed2721d3475232a6380e6b9853bd1647aec016440c0ca7ceb950daf1144f8db9814ab43cf33cc0ebef7fc91e9e775c9e874

        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libelf

        CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --program-prefix="eu-" --disable-debuginfod --enable-libdebuginfod=dummy
        BUILD_COMMAND ${__MAKE}

        TEST_COMMAND ${__MAKE} check
        TEST_BEFORE_INSTALL true

        INSTALL_COMMAND ${__MAKE} install

        DEPENDS libz
        )

ExternalProject_Get_Property(libelf INSTALL_DIR)

add_library(elf UNKNOWN IMPORTED)
set_target_properties(elf
        PROPERTIES
            IMPORTED_LOCATION ${INSTALL_DIR}/lib/libelf.a
        )
add_dependencies(elf libelf)
include_directories(BEFORE ${INSTALL_DIR}/include)