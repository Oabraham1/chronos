/**
 * @file gpu_partition.cpp
 * @brief Implementation of the GPUPartition class.
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

#include "core/gpu_partition.h"

namespace chronos {
namespace core {

GPUPartition::GPUPartition()
    : deviceId(nullptr),
      memoryFraction(0.0f),
      duration(std::chrono::seconds(0)),
      startTime(std::chrono::system_clock::now()),
      active(false),
      processId(0) {}

bool GPUPartition::isExpired() const {
    if (!active) {
        return true;
    }

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

    return elapsed >= duration;
}

int GPUPartition::getRemainingTime() const {
    if (!active) {
        return 0;
    }

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

    if (elapsed >= duration) {
        return 0;
    }

    return static_cast<int>((duration - elapsed).count());
}

}  // namespace core
}  // namespace chronos
