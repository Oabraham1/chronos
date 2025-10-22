/**
 * @file gpu_partition.h
 * @brief GPU partition data structures
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

#ifndef CHRONOS_GPU_PARTITION_H
#define CHRONOS_GPU_PARTITION_H

#include <chrono>
#include <string>

#include "platform/opencl_include.h"

namespace chronos {
namespace core {

/**
 * @class GPUPartition
 * @brief Represents a GPU partition allocation
 */
class GPUPartition {
   public:
    /**
     * @brief Default constructor
     */
    GPUPartition();

    /**
     * @brief Check if the partition has expired
     * @return True if expired, false otherwise
     */
    bool isExpired() const;

    /**
     * @brief Get remaining time in seconds
     * @return Seconds remaining until expiration
     */
    int getRemainingTime() const;

    cl_device_id deviceId;
    float memoryFraction;
    std::chrono::seconds duration;
    std::chrono::system_clock::time_point startTime;
    bool active;
    std::string partitionId;
    int processId;
    std::string username;
};

}  // namespace core
}  // namespace chronos

#endif
