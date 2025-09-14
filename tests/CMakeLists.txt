# Copyright (c) 2025 Ojima Abraham
# Licensed under the Business Source License 1.1
# Additional Use Grant: Production use permitted if gross revenue < $2,000/month
# Change Date: 2029-09-14, converts to GPL v3.0
# See LICENSE file for details

cmake_minimum_required(VERSION 3.10)

# Device info test
add_executable(test_device_info test_device_info.cpp)
target_link_libraries(test_device_info chronos)
if(NOT SKIP_OPENCL_TESTS)
    add_test(NAME DeviceInfoTest 
             COMMAND test_device_info
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
else()
    message(STATUS "Skipping OpenCL-dependent DeviceInfoTest in CI environment")
endif()

# Lock file test
add_executable(test_lock_file test_lock_file.cpp)
target_link_libraries(test_lock_file chronos)
add_test(NAME LockFileTest 
         COMMAND test_lock_file
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Partitioner test
add_executable(test_partitioner test_partitioner.cpp)
target_link_libraries(test_partitioner chronos)
if(NOT SKIP_OPENCL_TESTS)
    add_test(NAME PartitionerTest 
             COMMAND test_partitioner
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
else()
    message(STATUS "Skipping OpenCL-dependent PartitionerTest in CI environment")
endif()

# Special handling for MSVC which puts executables in a configuration subdirectory
if(MSVC)
    set_tests_properties(LockFileTest PROPERTIES
        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin/Release;$ENV{PATH}")
    
    if(NOT SKIP_OPENCL_TESTS)
        set_tests_properties(DeviceInfoTest PartitionerTest PROPERTIES
            ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin/Release;$ENV{PATH}")
    endif()
endif()