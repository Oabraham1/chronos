/**
 * @file opencl_backend.h
 * @brief OpenCL execution backend for cross-vendor GPU support
 *
 * This backend provides time-sliced GPU partitioning using OpenCL.
 * It works on any GPU with OpenCL support (NVIDIA, AMD, Intel, Apple).
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

#ifndef CHRONOS_OPENCL_BACKEND_H
#define CHRONOS_OPENCL_BACKEND_H

#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "backends/execution_backend.h"
#include "core/device_info.h"
#include "core/memory_enforcer.h"
#include "platform/opencl_include.h"
#include "utils/lock_file.h"

namespace chronos {
namespace backends {

/**
 * @class OpenCLBackend
 * @brief OpenCL-based execution backend for cross-vendor GPU support
 *
 * This backend provides time-sliced GPU partitioning. Multiple partitions
 * share the GPU via context switching, not true parallel execution.
 *
 * Use NVIDIA MPS backend for true concurrent execution on NVIDIA GPUs.
 */
class OpenCLBackend : public ExecutionBackend {
   public:
    /**
     * @brief Check if OpenCL is available on this system
     * @return true if OpenCL runtime is available
     */
    static bool checkAvailable();

    OpenCLBackend();
    ~OpenCLBackend() override;

    // Prevent copying
    OpenCLBackend(const OpenCLBackend&) = delete;
    OpenCLBackend& operator=(const OpenCLBackend&) = delete;

    // =========================================================================
    // ExecutionBackend Interface
    // =========================================================================

    std::string getName() const override { return "OpenCL"; }

    ExecutionMode getExecutionMode() const override { return ExecutionMode::TIME_SLICED; }

    bool isAvailable() const override { return available_; }

    std::string getDescription() const override {
        return "OpenCL backend (time-sliced execution, cross-vendor)";
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
    // Internal device info (keeps OpenCL handles)
    struct InternalDevice {
        cl_device_id clDeviceId;
        BackendDeviceInfo info;
        uint64_t allocatedMemory;  // Currently allocated memory

        InternalDevice() : clDeviceId(nullptr), allocatedMemory(0) {}
    };

    // Internal partition info
    struct InternalPartition {
        BackendPartition info;
        cl_device_id clDeviceId;

        InternalPartition() : clDeviceId(nullptr) {}
    };

    // =========================================================================
    // Private Methods
    // =========================================================================

    void initializeDevices();
    void initializeEnforcers();
    std::string generatePartitionId();
    void monitorPartitions();
    void releasePartitionResources(InternalPartition& partition);
    int getDeviceIndexByClId(cl_device_id deviceId) const;
    bool canAccessGPU(int deviceIndex, float memoryFraction) const;

    // =========================================================================
    // Member Variables
    // =========================================================================

    bool available_;
    bool initialized_;
    bool running_;

    cl_platform_id platform_;
    cl_context context_;

    std::vector<InternalDevice> devices_;
    std::map<int, std::unique_ptr<core::MemoryEnforcer>> enforcers_;
    std::vector<InternalPartition> partitions_;

    std::string lockFilePath_;
    std::unique_ptr<utils::LockFile> lockFile_;

    mutable std::mutex mutex_;
    std::thread monitorThread_;
};

}  // namespace backends
}  // namespace chronos

#endif  // CHRONOS_OPENCL_BACKEND_H
