cmake_minimum_required(VERSION 3.14)
project(conty
        VERSION 1.0.0
        DESCRIPTION "Primitive non-portable container library for Linux"
        LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)
# The library must have GNU_SOURCE enabled
string(APPEND CMAKE_C_FLAGS " -D_GNU_SOURCE")

add_library(conty STATIC)
target_sources(conty
    PRIVATE
        log.c
        log.h
        resource.h
        safestring.h
        clone.h
        clone.c
        sync.c
        sync.h
        mount.h
        mount.c
        namespace.h
        user.h
        user.c
        queue.h
        oci.h
        oci.c
        json.c
        container.h
        container.c
    PUBLIC
        ../include/conty/conty.h)

# We want our test executables to have access to private interfaces
# so we add a flag to enable this during the build process
target_include_directories(conty
        PUBLIC
            ../include
            "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:TEST>>:${CMAKE_CURRENT_LIST_DIR}>>")

target_link_libraries(conty PRIVATE json-c)