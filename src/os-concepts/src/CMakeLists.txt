cmake_minimum_required(VERSION 3.14)

add_executable(proc-credentials proc-credentials.c)
target_link_libraries(proc-credentials cap)

add_executable(capabilities-print capabilities-print.c)
target_link_libraries(capabilities-print cap)


add_executable(clone clone.c)
add_executable(exit-normal exit-normal.c)
add_executable(exit-abnormal exit-abnormal.c)