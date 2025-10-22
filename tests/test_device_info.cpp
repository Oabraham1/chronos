/**
 * @file test_device_info.cpp
 * @brief Tests for the DeviceInfo class.
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

#include "core/device_info.h"

/**
 * @brief Test creation and basic functionality of DeviceInfo
 * @return 0 if successful, non-zero otherwise
 */
int testDeviceInfoBasic() {
    // Get a device to use for testing
    cl_int err;
    cl_uint numPlatforms;
    err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        std::cout << "No OpenCL platforms found - skipping test in CI environment" << std::endl;
        return 0;
    }

    std::vector<cl_platform_id> platforms(numPlatforms);
    err = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to get OpenCL platform IDs" << std::endl;
        return 1;
    }

    cl_uint numDevices;
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    if (err != CL_SUCCESS || numDevices == 0) {
        std::cout << "No OpenCL devices found - skipping test in CI environment" << std::endl;
        return 0;
    }

    std::vector<cl_device_id> devices(numDevices);
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to get OpenCL device IDs" << std::endl;
        return 1;
    }

    // Create a DeviceInfo instance
    chronos::core::DeviceInfo deviceInfo(devices[0]);

    // Verify basic properties
    assert(!deviceInfo.name.empty());
    assert(deviceInfo.type != 0);
    assert(deviceInfo.totalMemory > 0);
    assert(deviceInfo.availableMemory > 0);
    assert(!deviceInfo.vendor.empty());
    assert(!deviceInfo.version.empty());
    assert(!deviceInfo.getDeviceTypeString().empty());

    // Test default constructor
    chronos::core::DeviceInfo emptyDeviceInfo;
    assert(emptyDeviceInfo.id == nullptr);
    assert(emptyDeviceInfo.name.empty());
    assert(emptyDeviceInfo.type == 0);
    assert(emptyDeviceInfo.totalMemory == 0);
    assert(emptyDeviceInfo.availableMemory == 0);

    std::cout << "DeviceInfo basic test passed" << std::endl;
    return 0;
}

/**
 * @brief Main entry point
 * @return Exit code
 */
int main() {
    int result = 0;

    result += testDeviceInfoBasic();

    return result;
}
