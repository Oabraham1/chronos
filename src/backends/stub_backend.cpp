/**
 * @file stub_backend.cpp
 * @brief Stub execution backend implementation
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

#include "backends/stub_backend.h"

#include <algorithm>
#include <sstream>

namespace chronos {
namespace backends {

StubBackend::StubBackend() : initialized_(false), partitionCounter_(0) {}

bool StubBackend::initialize() {
    if (initialized_) {
        return true;
    }

    // Create a fake device for testing
    BackendDeviceInfo fakeDevice;
    fakeDevice.index = 0;
    fakeDevice.name = "Stub GPU (Testing Only)";
    fakeDevice.vendor = "Chronos";
    fakeDevice.driverVersion = "1.0.0";
    fakeDevice.totalMemory = 8ULL * 1024 * 1024 * 1024;  // 8 GB
    fakeDevice.availableMemory = fakeDevice.totalMemory;
    fakeDevice.deviceType = "GPU";
    fakeDevice.supportsCompute = true;

    fakeDevices_.push_back(fakeDevice);
    initialized_ = true;
    return true;
}

void StubBackend::shutdown() {
    partitions_.clear();
    fakeDevices_.clear();
    initialized_ = false;
}

int StubBackend::getDeviceCount() const { return static_cast<int>(fakeDevices_.size()); }

std::vector<BackendDeviceInfo> StubBackend::getDevices() const { return fakeDevices_; }

std::string StubBackend::createPartition(int deviceIndex, float memoryFraction, int durationSeconds,
                                         const std::string& username) {
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(fakeDevices_.size())) {
        setError("Invalid device index");
        return "";
    }

    if (memoryFraction <= 0.0f || memoryFraction > 1.0f) {
        setError("Invalid memory fraction");
        return "";
    }

    auto& device = fakeDevices_[deviceIndex];
    uint64_t requestedMemory = static_cast<uint64_t>(device.totalMemory * memoryFraction);

    if (requestedMemory > device.availableMemory) {
        setError("Not enough memory");
        return "";
    }

    std::stringstream ss;
    ss << "stub_partition_" << (++partitionCounter_);
    std::string partitionId = ss.str();

    BackendPartition partition;
    partition.partitionId = partitionId;
    partition.deviceIndex = deviceIndex;
    partition.memoryFraction = memoryFraction;
    partition.duration = std::chrono::seconds(durationSeconds);
    partition.startTime = std::chrono::system_clock::now();
    partition.active = true;
    partition.processId = 0;
    partition.username = username;

    partitions_.push_back(partition);
    device.availableMemory -= requestedMemory;

    return partitionId;
}

bool StubBackend::releasePartition(const std::string& partitionId) {
    for (auto it = partitions_.begin(); it != partitions_.end(); ++it) {
        if (it->partitionId == partitionId) {
            if (it->deviceIndex >= 0 && it->deviceIndex < static_cast<int>(fakeDevices_.size())) {
                uint64_t freedMemory = static_cast<uint64_t>(
                    fakeDevices_[it->deviceIndex].totalMemory * it->memoryFraction);
                fakeDevices_[it->deviceIndex].availableMemory += freedMemory;
            }
            partitions_.erase(it);
            return true;
        }
    }
    setError("Partition not found");
    return false;
}

std::vector<BackendPartition> StubBackend::listPartitions() const {
    std::vector<BackendPartition> active;
    for (const auto& p : partitions_) {
        if (p.active && !p.isExpired()) {
            active.push_back(p);
        }
    }
    return active;
}

float StubBackend::getAvailablePercentage(int deviceIndex) const {
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(fakeDevices_.size())) {
        return -1.0f;
    }
    const auto& device = fakeDevices_[deviceIndex];
    return 100.0f * (static_cast<float>(device.availableMemory) / device.totalMemory);
}

}  // namespace backends
}  // namespace chronos
