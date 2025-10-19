cmake_minimum_required(VERSION 3.10)

add_executable(test_device_info test_device_info.cpp)
target_link_libraries(test_device_info chronos)
if(NOT SKIP_OPENCL_TESTS)
    add_test(NAME DeviceInfoTest
             COMMAND test_device_info
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
else()
    message(STATUS "Skipping OpenCL-dependent DeviceInfoTest in CI environment")
endif()

add_executable(test_lock_file test_lock_file.cpp)
target_link_libraries(test_lock_file chronos)
add_test(NAME LockFileTest
         COMMAND test_lock_file
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(test_partitioner test_partitioner.cpp)
target_link_libraries(test_partitioner chronos)
if(NOT SKIP_OPENCL_TESTS)
    add_test(NAME PartitionerTest
             COMMAND test_partitioner
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
else()
    message(STATUS "Skipping OpenCL-dependent PartitionerTest in CI environment")
endif()

add_executable(test_memory_enforcer test_memory_enforcer.cpp)
target_link_libraries(test_memory_enforcer chronos)
if(NOT SKIP_OPENCL_TESTS)
    add_test(NAME MemoryEnforcerTest
             COMMAND test_memory_enforcer
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
else()
    message(STATUS "Skipping OpenCL-dependent MemoryEnforcerTest in CI environment")
endif()

add_executable(test_user_permissions test_user_permissions.cpp)
target_link_libraries(test_user_permissions chronos)
if(NOT SKIP_OPENCL_TESTS)
    add_test(NAME UserPermissionsTest
             COMMAND test_user_permissions
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
else()
    message(STATUS "Skipping OpenCL-dependent UserPermissionsTest in CI environment")
endif()

if(MSVC)
    set_tests_properties(LockFileTest PROPERTIES
        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin/Release;$ENV{PATH}")

    if(NOT SKIP_OPENCL_TESTS)
        set_tests_properties(DeviceInfoTest PartitionerTest MemoryEnforcerTest UserPermissionsTest PROPERTIES
            ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin/Release;$ENV{PATH}")
    endif()
endif()
