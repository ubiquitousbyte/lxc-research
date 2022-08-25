
find_package(PkgConfig)
pkg_check_modules(PkgConfig_LibElf QUIET libelf)

find_path(LIBELF_INCLUDE_DIRS
        NAMES
        libelf.h
        PATHS
        ${PkgConfig_LibElf_INCLUDE_DIRS}
        /usr/include
        /usr/include/libelf
        /usr/local/include
        /usr/local/include/libelf
        /opt/local/include
        /opt/local/include/libelf
        /sw/include
        /sw/include/libelf
        ENV CPATH)

find_library(LIBELF_LIBRARIES
        NAMES
        elf
        PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
        ${PkgConfig_LibElf_LIBRARY_DIRS}
        ENV LIBRARY_PATH
        ENV LD_LIBRARY_PATH)

include(FindPackageHandleStandardArgs)


find_package_handle_standard_args(LibElf
        "Please install the libelf development package"
        LIBELF_LIBRARIES
        LIBELF_INCLUDE_DIRS)

set(CMAKE_REQUIRED_LIBRARIES elf)

include(CheckCXXSourceCompiles)

check_cxx_source_compiles(
    "#include <libelf.h>
    int main() {
        Elf *e = (Elf*)0;
        size_t sz;
        elf_getshdrstrndx(e, &sz);
        return 0;
    }"
    ELF_GETSHDRSTRNDX)

set(CMAKE_REQUIRED_LIBRARIES)