cmake_minimum_required(VERSION 3.14)
enable_testing()

include(FetchContent)
FetchContent_Declare(googletest
    URL 
    https://github.com/google/googletest/archive/58d77fa8070e8cec2dc1ed015d66b454c8d78850.zip)
FetchContent_MakeAvailable(googletest)

add_executable(oci-test oci-test.cpp)
target_link_libraries(oci-test PRIVATE conty gtest_main)

add_executable(context-test context-test.cpp)
target_link_libraries(context-test PRIVATE conty gtest_main)

include(GoogleTest)
gtest_discover_tests(oci-test)
gtest_discover_tests(context-test)