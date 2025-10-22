/**
 * @file test_memory_enforcer.cpp
 * @brief Tests for memory enforcer functionality
 *
 * Copyright (c) 2025 Ojima Abraham
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Ojima Abraham
 * @date 2025
 */

#include <cassert>
#include <iostream>
#include <vector>

#include "core/memory_enforcer.h"
#include "platform/opencl_include.h"

// Only include real OpenCL headers if not skipping OpenCL tests
#ifndef SKIP_OPENCL_TESTS
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#endif

int testBasicAllocation() {
#ifdef SKIP_OPENCL_TESTS
    std::cout << "Skipping testBasicAllocation - OpenCL not available in CI" << std::endl;
    return 0;
#else
    cl_int err;
    cl_uint numPlatforms;

    err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        std::cout << "No OpenCL platforms - skipping test" << std::endl;
        return 0;
    }

    std::vector<cl_platform_id> platforms(numPlatforms);
    clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);

    cl_uint numDevices;
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    if (err != CL_SUCCESS || numDevices == 0) {
        std::cout << "No OpenCL devices - skipping test" << std::endl;
        return 0;
    }

    std::vector<cl_device_id> devices(numDevices);
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);

    cl_context_properties props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[0], 0};
    cl_context context = clCreateContext(props, 1, &devices[0], nullptr, nullptr, &err);
    assert(err == CL_SUCCESS);

    chronos::core::MemoryEnforcer enforcer(devices[0], context);

    size_t limit = 1024 * 1024 * 1024;  // 1 GB
    assert(enforcer.allocatePartition("test_001", limit));

    assert(enforcer.canAllocate("test_001", 512 * 1024 * 1024));

    // Fix: Use proper 64-bit arithmetic to avoid overflow
    size_t largeSize = static_cast<size_t>(2048) * 1024 * 1024;  // 2 GB
    assert(!enforcer.canAllocate("test_001", largeSize));

    assert(enforcer.getMemoryLimit("test_001") == limit);
    assert(enforcer.getCurrentUsage("test_001") == 0);

    enforcer.releasePartition("test_001");

    clReleaseContext(context);

    std::cout << "Basic allocation test passed" << std::endl;
    return 0;
#endif
}

int testBufferTracking() {
#ifdef SKIP_OPENCL_TESTS
    std::cout << "Skipping testBufferTracking - OpenCL not available in CI" << std::endl;
    return 0;
#else
    cl_int err;
    cl_uint numPlatforms;

    err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        std::cout << "No OpenCL platforms - skipping test" << std::endl;
        return 0;
    }

    std::vector<cl_platform_id> platforms(numPlatforms);
    clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);

    cl_uint numDevices;
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    if (err != CL_SUCCESS || numDevices == 0) {
        std::cout << "No OpenCL devices - skipping test" << std::endl;
        return 0;
    }

    std::vector<cl_device_id> devices(numDevices);
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);

    cl_context_properties props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[0], 0};
    cl_context context = clCreateContext(props, 1, &devices[0], nullptr, nullptr, &err);
    assert(err == CL_SUCCESS);

    chronos::core::MemoryEnforcer enforcer(devices[0], context);

    size_t limit = 100 * 1024 * 1024;  // 100 MB
    assert(enforcer.allocatePartition("test_002", limit));

    size_t bufferSize = 50 * 1024 * 1024;  // 50 MB
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, bufferSize, nullptr, &err);
    assert(err == CL_SUCCESS);

    assert(enforcer.trackBuffer("test_002", buffer, bufferSize));
    assert(enforcer.getCurrentUsage("test_002") == bufferSize);

    // Try to allocate more than the limit
    size_t excessSize = 60 * 1024 * 1024;  // 60 MB (would exceed 100 MB limit)
    assert(!enforcer.trackBuffer("test_002", buffer, excessSize));

    assert(enforcer.releaseBuffer("test_002", buffer));
    assert(enforcer.getCurrentUsage("test_002") == 0);

    enforcer.releasePartition("test_002");
    clReleaseContext(context);

    std::cout << "Buffer tracking test passed" << std::endl;
    return 0;
#endif
}

int testMultiplePartitions() {
#ifdef SKIP_OPENCL_TESTS
    std::cout << "Skipping testMultiplePartitions - OpenCL not available in CI" << std::endl;
    return 0;
#else
    cl_int err;
    cl_uint numPlatforms;

    err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        std::cout << "No OpenCL platforms - skipping test" << std::endl;
        return 0;
    }

    std::vector<cl_platform_id> platforms(numPlatforms);
    clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);

    cl_uint numDevices;
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    if (err != CL_SUCCESS || numDevices == 0) {
        std::cout << "No OpenCL devices - skipping test" << std::endl;
        return 0;
    }

    std::vector<cl_device_id> devices(numDevices);
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);

    cl_context_properties props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[0], 0};
    cl_context context = clCreateContext(props, 1, &devices[0], nullptr, nullptr, &err);
    assert(err == CL_SUCCESS);

    chronos::core::MemoryEnforcer enforcer(devices[0], context);

    assert(enforcer.allocatePartition("part_001", 100 * 1024 * 1024));
    assert(enforcer.allocatePartition("part_002", 200 * 1024 * 1024));
    assert(enforcer.allocatePartition("part_003", 150 * 1024 * 1024));

    auto partitions = enforcer.getActivePartitions();
    assert(partitions.size() == 3);

    enforcer.releasePartition("part_001");
    partitions = enforcer.getActivePartitions();
    assert(partitions.size() == 2);

    enforcer.releasePartition("part_002");
    enforcer.releasePartition("part_003");
    partitions = enforcer.getActivePartitions();
    assert(partitions.empty());

    clReleaseContext(context);

    std::cout << "Multiple partitions test passed" << std::endl;
    return 0;
#endif
}

int main() {
    int result = 0;

    std::cout << "=== Testing Memory Enforcer ===" << std::endl;

#ifdef SKIP_OPENCL_TESTS
    std::cout << "Note: Running in CI mode - OpenCL tests will be skipped" << std::endl;
#endif

    result += testBasicAllocation();
    result += testBufferTracking();
    result += testMultiplePartitions();

    if (result == 0) {
        std::cout << "All memory enforcer tests passed!" << std::endl;
    } else {
        std::cout << result << " tests failed" << std::endl;
    }

    return result;
}
