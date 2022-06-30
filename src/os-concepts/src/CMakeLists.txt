cmake_minimum_required(VERSION 3.14)

add_executable(proc-credentials proc-credentials.c)
target_link_libraries(proc-credentials cap)

add_executable(clone clone.c)