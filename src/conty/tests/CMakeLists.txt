cmake_minimum_required(VERSION 3.14)
enable_testing()

include(FetchContent)
FetchContent_Declare(googletest
    URL
    https://github.com/google/googletest/archive/58d77fa8070e8cec2dc1ed015d66b454c8d78850.zip)
FetchContent_MakeAvailable(googletest)

add_executable(hook-test hook-test.cpp)
target_link_libraries(hook-test PRIVATE conty gtest_main)
set_property(TARGET hook-test PROPERTY TEST 1)

add_executable(conf-test conf-test.cpp)
target_link_libraries(conf-test PRIVATE conty gtest_main)
set_property(TARGET conf-test PROPERTY TEST 1)

add_executable(user-test user-test.cpp)
target_link_libraries(user-test PRIVATE conty gtest_main)
set_property(TARGET user-test PROPERTY TEST 1)

add_executable(network-test network-test.cpp)
target_link_libraries(network-test PRIVATE conty gtest_main)
set_property(TARGET network-test PROPERTY TEST 1)

add_executable(container-test container-test.cpp)
target_link_libraries(container-test PRIVATE conty gtest_main)
set_property(TARGET container-test PROPERTY TEST 1)

include(GoogleTest)
gtest_discover_tests(hook-test)
gtest_discover_tests(conf-test)
gtest_discover_tests(user-test)
gtest_discover_tests(network-test)