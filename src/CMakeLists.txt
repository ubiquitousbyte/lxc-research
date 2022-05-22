cmake_minimum_required(VERSION 3.14)
include(${CMAKE_CURRENT_LIST_DIR}/project-meta-info.in)
project(lxcr
    LANGUAGES C 
    VERSION ${LXCR_VERSION} 
    DESCRIPTION ${LXCR_VERSION})

set(CMAKE_C_STANDARD 99)