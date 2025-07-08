/**
 * @file device_info.cpp
 * @brief Implementation of the DeviceInfo class.
 *
 * This file provides the implementation for the DeviceInfo class, which is
 * responsible for querying and storing information about OpenCL devices.
 * This includes details such as the device name, vendor, memory, and version,
 * which are essential for the operation of the Chronos partitioner.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#include "core/device_info.h"
#include <iostream>

namespace chronos {
namespace core {

DeviceInfo::DeviceInfo()
    : id(nullptr), type(0), totalMemory(0), availableMemory(0) {}

DeviceInfo::DeviceInfo(cl_device_id deviceId)
    : id(deviceId), type(0), totalMemory(0), availableMemory(0) {
  loadDeviceInfo();
}

bool DeviceInfo::loadDeviceInfo() {
#ifdef SKIP_OPENCL_TESTS
  // Return mock values when OpenCL tests are skipped
  name = "Mock Device";
  vendor = "Mock Vendor";
  version = "Mock OpenCL 1.2";
  type = CL_DEVICE_TYPE_GPU;
  totalMemory = 1024 * 1024 * 1024; // 1GB
  availableMemory = totalMemory;
  return true;

#else
  cl_int err;

  if (id == nullptr) {
    std::cerr << "Device ID is null" << std::endl;
    return false;
  }

  char deviceName[256];
  err = clGetDeviceInfo(id, CL_DEVICE_NAME, sizeof(deviceName), deviceName,
                        nullptr);
  if (err == CL_SUCCESS) {
    name = deviceName;
  } else {
    name = "Unknown";
    std::cerr << "Failed to get device name: " << err << std::endl;
  }

  err = clGetDeviceInfo(id, CL_DEVICE_TYPE, sizeof(type), &type, nullptr);
  if (err != CL_SUCCESS) {
    std::cerr << "Failed to get device type: " << err << std::endl;
    type = 0;
  }

  cl_ulong deviceMemory;
  err = clGetDeviceInfo(id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(deviceMemory),
                        &deviceMemory, nullptr);
  if (err == CL_SUCCESS) {
    totalMemory = deviceMemory;
    availableMemory = deviceMemory;
  } else {
    std::cerr << "Failed to get device memory: " << err << std::endl;
    totalMemory = 0;
    availableMemory = 0;
  }

  char vendorName[256];
  err = clGetDeviceInfo(id, CL_DEVICE_VENDOR, sizeof(vendorName), vendorName,
                        nullptr);
  if (err == CL_SUCCESS) {
    vendor = vendorName;
  } else {
    std::cerr << "Failed to get device vendor: " << err << std::endl;
    vendor = "Unknown";
  }

  char version[256];
  err =
      clGetDeviceInfo(id, CL_DEVICE_VERSION, sizeof(version), version, nullptr);
  if (err == CL_SUCCESS) {
    this->version = version;
  } else {
    std::cerr << "Failed to get device version: " << err << std::endl;
    this->version = "Unknown";
  }

  return true;
#endif
}

std::string DeviceInfo::getDeviceTypeString() const {
  std::string result;

  if (type & CL_DEVICE_TYPE_CPU)
    result += "CPU ";
  if (type & CL_DEVICE_TYPE_GPU)
    result += "GPU ";
  if (type & CL_DEVICE_TYPE_ACCELERATOR)
    result += "Accelerator ";
  if (type & CL_DEVICE_TYPE_DEFAULT)
    result += "Default ";

  return result.empty() ? "Unknown" : result;
}

} // namespace core
} // namespace chronos