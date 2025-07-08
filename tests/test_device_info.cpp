/**
 * @file test_device_info.cpp
 * @brief Tests for the DeviceInfo class.
 *
 * This file contains unit tests for the DeviceInfo class, ensuring that
 * it correctly queries and stores information about OpenCL devices.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#include "core/device_info.h"
#include <cassert>
#include <iostream>
#include <vector>

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
    std::cout << "No OpenCL platforms found - skipping test in CI environment"
              << std::endl;
    return 0;
  }

  std::vector<cl_platform_id> platforms(numPlatforms);
  err = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
  if (err != CL_SUCCESS) {
    std::cerr << "Failed to get OpenCL platform IDs" << std::endl;
    return 1;
  }

  cl_uint numDevices;
  err =
      clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
  if (err != CL_SUCCESS || numDevices == 0) {
    std::cout << "No OpenCL devices found - skipping test in CI environment"
              << std::endl;
    return 0;
  }

  std::vector<cl_device_id> devices(numDevices);
  err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices,
                       devices.data(), nullptr);
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