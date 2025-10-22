/**
 * @file device_info.h
 * @brief Device information structures
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

#ifndef CHRONOS_DEVICE_INFO_H
#define CHRONOS_DEVICE_INFO_H

#include <string>

#include "platform/opencl_include.h"

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

}  // namespace core
}  // namespace chronos

#endif
