/**
 * @file mps_backend.h
 * @brief NVIDIA MPS execution backend for true concurrent GPU execution
 *
 * NVIDIA MPS (Multi-Process Service) allows multiple CUDA processes to
 * share the GPU's streaming multiprocessors concurrently, rather than
 * time-slicing.
 *
 * Requirements:
 * - NVIDIA GPU (Volta or newer recommended)
 * - CUDA drivers installed
 * - nvidia-cuda-mps-control available
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

#ifndef CHRONOS_MPS_BACKEND_H
#define CHRONOS_MPS_BACKEND_H

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
 * @class MPSBackend
 * @brief NVIDIA MPS-based execution backend for true concurrent execution
 *
 * This backend uses NVIDIA's Multi-Process Service (MPS) to allow multiple
 * GPU partitions to execute truly in parallel, sharing streaming multiprocessors.
 *
 * Key features:
 * - True concurrent execution (not time-sliced)
 * - Hardware-enforced memory limits via MPS
 * - Thread percentage limits per partition
 */
class MPSBackend : public ExecutionBackend {
   public:
    /**
     * @brief Check if MPS is available on this system
     * @return true if nvidia-smi and MPS control are available
     */
    static bool checkAvailable();

    MPSBackend();
    ~MPSBackend() override;

    // Prevent copying
    MPSBackend(const MPSBackend&) = delete;
    MPSBackend& operator=(const MPSBackend&) = delete;

    // =========================================================================
    // ExecutionBackend Interface
    // =========================================================================

    std::string getName() const override { return "NVIDIA MPS"; }
    ExecutionMode getExecutionMode() const override { return ExecutionMode::CONCURRENT; }
    bool isAvailable() const override { return available_; }
    std::string getDescription() const override {
        return "NVIDIA MPS backend (true concurrent execution)";
    }

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
    // Internal partition with MPS-specific data
    struct MPSPartition {
        BackendPartition info;
        int threadPercentage;  // CUDA_MPS_ACTIVE_THREAD_PERCENTAGE
    };

    // =========================================================================
    // Private Methods
    // =========================================================================

    bool queryNvidiaDevices();
    bool ensureMPSServerRunning();
    void stopMPSServer();
    std::string generatePartitionId();
    void monitorPartitions();
    void releasePartitionResources(MPSPartition& partition);
    int parseGPUMemory(const std::string& memoryStr);

    // =========================================================================
    // Member Variables
    // =========================================================================

    bool available_;
    bool initialized_;
    bool running_;
    bool ownsServer_;  // True if we started the MPS server

    std::vector<BackendDeviceInfo> devices_;
    std::vector<MPSPartition> partitions_;

    std::string lockFilePath_;
    std::unique_ptr<utils::LockFile> lockFile_;

    mutable std::mutex mutex_;
    std::thread monitorThread_;
};

}  // namespace backends
}  // namespace chronos

#endif  // CHRONOS_MPS_BACKEND_H
