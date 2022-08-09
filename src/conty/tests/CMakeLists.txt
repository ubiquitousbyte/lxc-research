cmake_minimum_required(VERSION 3.14)
enable_testing()

include(FetchContent)
FetchContent_Declare(googletest
    URL
    https://github.com/google/googletest/archive/58d77fa8070e8cec2dc1ed015d66b454c8d78850.zip)
FetchContent_MakeAvailable(googletest)

add_executable(hook-test hook-test.cpp)
target_link_libraries(hook-test PRIVATE conty gtest_main)

add_executable(conf-test conf-test.cpp)
target_link_libraries(conf-test PRIVATE conty gtest_main)

add_executable(container-test container-test.c)
target_link_libraries(container-test PRIVATE conty)
set_property(TARGET container-test PROPERTY TEST 1)

include(GoogleTest)
gtest_discover_tests(hook-test)
gtest_discover_tests(conf-test)
