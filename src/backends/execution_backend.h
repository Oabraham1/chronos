/**
 * @file execution_backend.h
 * @brief Abstract interface for GPU execution backends
 *
 * This file defines the ExecutionBackend interface that allows Chronos
 * to support multiple GPU execution strategies:
 * - NVIDIA MPS for true concurrent execution
 * - AMD ROCm for AMD GPU support
 * - OpenCL for cross-vendor time-sliced execution
 * - Stub for testing/no-GPU environments
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

#ifndef CHRONOS_EXECUTION_BACKEND_H
#define CHRONOS_EXECUTION_BACKEND_H

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace chronos {
namespace backends {

/**
 * @enum ExecutionMode
 * @brief Defines the execution mode of a backend
 */
enum class ExecutionMode {
    CONCURRENT = 0,   ///< True parallel execution (MPS, ROCm)
    TIME_SLICED = 1,  ///< Context switching between partitions (OpenCL)
    STUB = 2          ///< No-op fallback for testing
};

/**
 * @struct BackendDeviceInfo
 * @brief Backend-agnostic GPU device information
 *
 * This structure provides device information without depending on
 * OpenCL or any specific backend API.
 */
struct BackendDeviceInfo {
    int index;                  ///< Device index (0, 1, 2, ...)
    std::string name;           ///< Device name (e.g., "NVIDIA GeForce RTX 3080")
    std::string vendor;         ///< Vendor name (e.g., "NVIDIA", "AMD", "Intel")
    std::string driverVersion;  ///< Driver version string
    uint64_t totalMemory;       ///< Total device memory in bytes
    uint64_t availableMemory;   ///< Currently available memory in bytes
    std::string deviceType;     ///< Device type ("GPU", "CPU", "Accelerator")
    bool supportsCompute;       ///< Whether device supports compute operations

    BackendDeviceInfo() : index(0), totalMemory(0), availableMemory(0), supportsCompute(false) {}
};

/**
 * @struct BackendPartition
 * @brief Backend-agnostic partition information
 */
struct BackendPartition {
    std::string partitionId;                          ///< Unique partition identifier
    int deviceIndex;                                  ///< GPU device index
    float memoryFraction;                             ///< Fraction of GPU memory (0.0 - 1.0)
    std::chrono::seconds duration;                    ///< Allocation duration
    std::chrono::system_clock::time_point startTime;  ///< When partition was created
    bool active;                                      ///< Whether partition is active
    int processId;                                    ///< Owning process ID
    std::string username;                             ///< Owning username
    std::map<std::string, std::string> metadata;      ///< Backend-specific metadata

    BackendPartition() : deviceIndex(0), memoryFraction(0.0f), active(false), processId(0) {}

    /**
     * @brief Check if partition has expired
     */
    bool isExpired() const {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
        return elapsed >= duration;
    }

    /**
     * @brief Get remaining time in seconds
     */
    int getRemainingTime() const {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
        auto remaining = duration - elapsed;
        return remaining.count() > 0 ? static_cast<int>(remaining.count()) : 0;
    }
};

/**
 * @class ExecutionBackend
 * @brief Abstract base class for GPU execution backends
 *
 * All GPU backends (MPS, ROCm, OpenCL, Stub) must implement this interface.
 * The interface provides methods for:
 * - Backend identification and capability queries
 * - Device enumeration and information
 * - Partition lifecycle management
 * - Resource availability queries
 */
class ExecutionBackend {
   public:
    virtual ~ExecutionBackend() = default;

    // =========================================================================
    // Backend Identification
    // =========================================================================

    /**
     * @brief Get the backend name
     * @return Human-readable backend name (e.g., "NVIDIA MPS", "OpenCL")
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get the execution mode of this backend
     * @return CONCURRENT, TIME_SLICED, or STUB
     */
    virtual ExecutionMode getExecutionMode() const = 0;

    /**
     * @brief Check if this backend is available on the current system
     * @return true if the backend can be used
     */
    virtual bool isAvailable() const = 0;

    /**
     * @brief Get a human-readable description of the backend
     * @return Description string
     */
    virtual std::string getDescription() const { return getName() + " backend"; }

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * @brief Initialize the backend
     *
     * This method should be called before using any other methods.
     * It initializes any required resources (GPU context, MPS server, etc.)
     *
     * @return true if initialization succeeded
     */
    virtual bool initialize() = 0;

    /**
     * @brief Shutdown the backend
     *
     * Releases all resources and cleans up. Called automatically by destructor.
     */
    virtual void shutdown() = 0;

    // =========================================================================
    // Device Management
    // =========================================================================

    /**
     * @brief Get the number of available devices
     * @return Number of GPU devices
     */
    virtual int getDeviceCount() const = 0;

    /**
     * @brief Get information about all devices
     * @return Vector of device information structures
     */
    virtual std::vector<BackendDeviceInfo> getDevices() const = 0;

    /**
     * @brief Get information about a specific device
     * @param deviceIndex Device index
     * @return Device information, or empty struct if invalid index
     */
    virtual BackendDeviceInfo getDevice(int deviceIndex) const {
        auto devices = getDevices();
        if (deviceIndex >= 0 && deviceIndex < static_cast<int>(devices.size())) {
            return devices[deviceIndex];
        }
        return BackendDeviceInfo();
    }

    // =========================================================================
    // Partition Management
    // =========================================================================

    /**
     * @brief Create a new partition
     *
     * @param deviceIndex GPU device index
     * @param memoryFraction Fraction of GPU memory to allocate (0.0 - 1.0)
     * @param durationSeconds Duration in seconds
     * @param username Username to associate with partition
     * @return Partition ID on success, empty string on failure
     */
    virtual std::string createPartition(int deviceIndex, float memoryFraction, int durationSeconds,
                                        const std::string& username) = 0;

    /**
     * @brief Release a partition
     * @param partitionId ID of the partition to release
     * @return true if partition was released successfully
     */
    virtual bool releasePartition(const std::string& partitionId) = 0;

    /**
     * @brief List all active partitions
     * @return Vector of partition information structures
     */
    virtual std::vector<BackendPartition> listPartitions() const = 0;

    /**
     * @brief Get information about a specific partition
     * @param partitionId Partition ID
     * @return Partition information, or empty struct if not found
     */
    virtual BackendPartition getPartition(const std::string& partitionId) const {
        auto partitions = listPartitions();
        for (const auto& p : partitions) {
            if (p.partitionId == partitionId) {
                return p;
            }
        }
        return BackendPartition();
    }

    // =========================================================================
    // Resource Queries
    // =========================================================================

    /**
     * @brief Get available memory percentage for a device
     * @param deviceIndex Device index
     * @return Available memory as fraction (0.0 - 1.0), or -1 on error
     */
    virtual float getAvailablePercentage(int deviceIndex) const = 0;

    /**
     * @brief Get total memory for a device
     * @param deviceIndex Device index
     * @return Total memory in bytes
     */
    virtual uint64_t getTotalMemory(int deviceIndex) const {
        auto device = getDevice(deviceIndex);
        return device.totalMemory;
    }

    // =========================================================================
    // Error Handling
    // =========================================================================

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    virtual std::string getLastError() const { return lastError_; }

   protected:
    mutable std::string lastError_;  ///< Last error message

    /**
     * @brief Set the last error message
     * @param error Error message
     */
    void setError(const std::string& error) const { lastError_ = error; }
};

/**
 * @brief Convert ExecutionMode to string
 */
inline std::string executionModeToString(ExecutionMode mode) {
    switch (mode) {
        case ExecutionMode::CONCURRENT:
            return "concurrent";
        case ExecutionMode::TIME_SLICED:
            return "time_sliced";
        case ExecutionMode::STUB:
            return "stub";
        default:
            return "unknown";
    }
}

/**
 * @brief Convert string to ExecutionMode
 */
inline ExecutionMode stringToExecutionMode(const std::string& str) {
    if (str == "concurrent") return ExecutionMode::CONCURRENT;
    if (str == "time_sliced") return ExecutionMode::TIME_SLICED;
    if (str == "stub") return ExecutionMode::STUB;
    return ExecutionMode::STUB;
}

}  // namespace backends
}  // namespace chronos

#endif  // CHRONOS_EXECUTION_BACKEND_H
