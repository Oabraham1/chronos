/**
 * @file rocm_backend.h
 * @brief AMD ROCm execution backend for AMD GPU support
 *
 * This backend provides AMD GPU support via ROCm.
 *
 * Requirements:
 * - AMD GPU with ROCm support
 * - ROCm drivers installed
 * - rocm-smi available
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

#ifndef CHRONOS_ROCM_BACKEND_H
#define CHRONOS_ROCM_BACKEND_H

#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "backends/execution_backend.h"
#include "utils/lock_file.h"

namespace chronos {
namespace backends {

/**
 * @class ROCmBackend
 * @brief AMD ROCm-based execution backend
 *
 * This backend provides GPU partitioning for AMD GPUs using ROCm.
 * Uses rocm-smi for device enumeration and memory queries.
 */
class ROCmBackend : public ExecutionBackend {
   public:
    /**
     * @brief Check if ROCm is available on this system
     * @return true if rocm-smi is available
     */
    static bool checkAvailable();

    ROCmBackend();
    ~ROCmBackend() override;

    // Prevent copying
    ROCmBackend(const ROCmBackend&) = delete;
    ROCmBackend& operator=(const ROCmBackend&) = delete;

    // =========================================================================
    // ExecutionBackend Interface
    // =========================================================================

    std::string getName() const override { return "ROCm"; }
    ExecutionMode getExecutionMode() const override { return ExecutionMode::TIME_SLICED; }
    bool isAvailable() const override { return available_; }
    std::string getDescription() const override { return "AMD ROCm backend (AMD GPU support)"; }

    bool initialize() override;
    void shutdown() override;

    int getDeviceCount() const override { return static_cast<int>(devices_.size()); }
    std::vector<BackendDeviceInfo> getDevices() const override;
    BackendDeviceInfo getDevice(int deviceIndex) const override;

    std::string createPartition(int deviceIndex, float memoryFraction, int durationSeconds,
                                const std::string& username) override;
    bool releasePartition(const std::string& partitionId) override;
    std::vector<BackendPartition> listPartitions() const override;

    float getAvailablePercentage(int deviceIndex) const override;

   private:
    struct ROCmPartition {
        BackendPartition info;
    };

    bool queryROCmDevices();
    std::string generatePartitionId();
    void monitorPartitions();
    void releasePartitionResources(ROCmPartition& partition);

    bool available_;
    bool initialized_;
    bool running_;

    std::vector<BackendDeviceInfo> devices_;
    std::vector<ROCmPartition> partitions_;

    std::string lockFilePath_;
    std::unique_ptr<utils::LockFile> lockFile_;

    mutable std::mutex mutex_;
    std::thread monitorThread_;
};

}  // namespace backends
}  // namespace chronos

#endif  // CHRONOS_ROCM_BACKEND_H
