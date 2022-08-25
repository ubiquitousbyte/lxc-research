find_path(LIBBPF_INCLUDE_DIRS
        NAMES
        bpf/bpf.h
        bpf/btf.h
        bpf/libbpf.h
        PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        /sw/include
        ENV CPATH)

find_library(LIBBPF_LIBRARIES
        NAMES
        bpf
        PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
        ENV LIBRARY_PATH
        ENV LD_LIBRARY_PATH)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibBpf
        "Please install the libbpf development package"
        LIBBPF_LIBRARIES
        LIBBPF_INCLUDE_DIRS)

mark_as_advanced(LIBBPF_INCLUDE_DIRS LIBBPF_LIBRARIES)