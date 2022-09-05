cmake_minimum_required(VERSION 3.14)
project(conty 
    VERSION 1.0.0
    DESCRIPTION "Basic container runtime for Linux"
    LANGUAGES C CXX)

include(GNUInstallDirs)
include(ExternalProject)
include(FetchContent)

FetchContent_Declare(json-c
        GIT_REPOSITORY https://github.com/json-c/json-c.git
        GIT_TAG 2f2ddc1f2dbca56c874e8f9c31b5b963202d80e7)
FetchContent_MakeAvailable(json-c)

set(CONTY_PUBLIC_HEADERS ${CMAKE_CURRENT_LIST_DIR}/include)

# Bring in library and executable targets
add_subdirectory(lib)
add_subdirectory(src)

install(DIRECTORY ${CONTY_PUBLIC_HEADERS}/conty
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS conty conty-runtime)