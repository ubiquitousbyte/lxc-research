cmake_minimum_required(VERSION 3.14)
enable_testing()

add_executable(resource-test resource-test.c)
target_link_libraries(resource-test PUBLIC conty)
set_property(TARGET resource-test PROPERTY TEST 1)

add_executable(clone-test clone-test.c)
target_link_libraries(clone-test PUBLIC conty)
set_property(TARGET clone-test PROPERTY TEST 1)

add_executable(sync-test sync-test.c)
target_link_libraries(sync-test PUBLIC conty)
set_property(TARGET sync-test PROPERTY TEST 1)

add_executable(mount-test mount-test.c)
target_link_libraries(mount-test PUBLIC conty)
set_property(TARGET mount-test PROPERTY TEST 1)

add_executable(oci-test oci-test.c)
target_link_libraries(oci-test PUBLIC conty)
set_property(TARGET oci-test PROPERTY TEST 1)

add_executable(container-test container-test.c)
target_link_libraries(container-test PUBLIC conty)
set_property(TARGET container-test PROPERTY TEST 1)

add_executable(user-test user-test.c)
target_link_libraries(user-test PUBLIC conty)
set_property(TARGET user-test PROPERTY TEST 1)