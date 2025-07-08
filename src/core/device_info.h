/**
 * @file device_info.h
 * @brief Device information structures
 *
 * This file defines the DeviceInfo class, which encapsulates all the
 * necessary information about an OpenCL device, such as its name, vendor,
 * memory, and version.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#ifndef CHRONOS_DEVICE_INFO_H
#define CHRONOS_DEVICE_INFO_H

#include "platform/opencl_include.h"
#include <string>

namespace chronos {
namespace core {

/**
 * @class DeviceInfo
 * @brief Stores information about an OpenCL device
 */
class DeviceInfo {
public:
  /**
   * @brief Default constructor
   */
  DeviceInfo();

  /**
   * @brief Constructor with device ID
   * @param deviceId OpenCL device ID
   */
  explicit DeviceInfo(cl_device_id deviceId);

  /**
   * @brief Load device information
   *
   * Queries and loads all device properties from OpenCL.
   *
   * @return True if successful, false otherwise
   */
  bool loadDeviceInfo();

  /**
   * @brief Get device type as string
   * @return Human-readable device type
   */
  std::string getDeviceTypeString() const;

  cl_device_id id;
  std::string name;
  cl_device_type type;
  cl_ulong totalMemory;
  cl_ulong availableMemory;
  std::string vendor;
  std::string version;
};

} // namespace core
} // namespace chronos

#endif