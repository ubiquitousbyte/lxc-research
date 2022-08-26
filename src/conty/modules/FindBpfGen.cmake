if(NOT ZLIB_INCLUDE_DIRS OR NOT ZLIB_LIBRARIES)
    find_package(ZLIB REQUIRED)
endif()

if (NOT LIBELF_INCLUDE_DIRS OR NOT LIBELF_LIBRARIES)
    find_package(LibElf REQUIRED)
endif()

if (NOT LIBBPF_INCLUDE_DIRS OR NOT LIBBPF_LIBRARIES)
    find_package(LibBpf REQUIRED)
endif()

if(NOT BPFGEN_BPFTOOL)
    find_program(BPFGEN_BPFTOOL NAMES bpftool DOC "Path to bpftool" REQUIRED)
endif()

if(NOT BPFGEN_CLANG)
    # TODO: Make sure version is >= 10
    find_program(BPFGEN_CLANG NAMES clang DOC "Path to clang executable")
endif()

if (NOT BPFGEN_VMLINUX_DIR)
    set(BPFGEN_VMLINUX_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

set(BPFGEN_VMLINUX_H ${BPFGEN_VMLINUX_DIR}/vmlinux.h)

if (NOT EXISTS "${BPFGEN_VMLINUX_H}")
    # Add target that generates vmlinux.h with portable kernel symbols
    add_custom_command(
            OUTPUT ${BPFGEN_VMLINUX_H}
            COMMAND
                ${BPFGEN_BPFTOOL} btf dump file
                /sys/kernel/btf/vmlinux format c > ${BPFGEN_VMLINUX_H}
            DEPENDS ${BPFGEN_BPFTOOL}
            VERBATIM)
endif()

add_custom_target(bpfgen_vmlinux_h DEPENDS ${BPFGEN_VMLINUX_H})

macro(add_bpf_skeleton SKEL_NAME SOURCE_FILE)
    set(BPF_C_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE})
    set(BPF_O_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SKEL_NAME}.bpf.o)
    set(BPF_SKEL_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SKEL_NAME}.skel.h)
    set(OUTPUT_TARGET ${SKEL_NAME}_skel)

    add_custom_command(
            OUTPUT ${BPF_O_FILE}
            COMMAND
                ${BPFGEN_CLANG} -g -O2 -target bpf -D__TARGET_ARCH_x86
                -I${BPFGEN_VMLINUX_DIR} -isystem ${LIBBPF_INCLUDE_DIRS}
                -c ${BPF_C_FILE} -o ${BPF_O_FILE}
            VERBATIM
            DEPENDS ${BPF_C_FILE})

    add_custom_command(
            OUTPUT ${BPF_SKEL_FILE}
            COMMAND
                bash -c "${BPFGEN_BPFTOOL} gen skeleton ${BPF_O_FILE} \
                    > ${BPF_SKEL_FILE}"
            VERBATIM
            DEPENDS ${BPF_O_FILE} ${BPFGEN_VMLINUX_H})

    add_library(${OUTPUT_TARGET} INTERFACE)
    target_sources(${OUTPUT_TARGET} INTERFACE ${BPF_SKEL_FILE})
    target_include_directories(${OUTPUT_TARGET}
            INTERFACE
                ${CMAKE_CURRENT_BINARY_DIR}
                ${BPFGEN_VMLINUX_DIR}
            SYSTEM INTERFACE
                ${LIBBPF_INCLUDE_DIRS})
    target_link_libraries(${OUTPUT_TARGET}
            INTERFACE
                ${LIBBPF_LIBRARIES}
                ${ZLIB_LIBRARIES}
                ${LIBELF_LIBRARIES})
endmacro()