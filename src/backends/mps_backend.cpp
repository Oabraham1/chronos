/**
 * @file mps_backend.cpp
 * @brief NVIDIA MPS execution backend implementation
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

#include "backends/mps_backend.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "platform/platform.h"

namespace chronos {
namespace backends {

namespace {

// Helper function to execute a command and get output
std::string executeCommand(const std::string& cmd) {
    std::array<char, 4096> buffer;
    std::string result;

#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
#endif

    if (!pipe) {
        return "";
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}

// Check if a command exists
bool commandExists(const std::string& cmd) {
#ifdef _WIN32
    std::string check = "where " + cmd + " >nul 2>&1";
#else
    std::string check = "which " + cmd + " >/dev/null 2>&1";
#endif
    return system(check.c_str()) == 0;
}

}  // anonymous namespace

bool MPSBackend::checkAvailable() {
    // Check for nvidia-smi
    if (!commandExists("nvidia-smi")) {
        return false;
    }

    // Check for MPS control (might not be in PATH but should exist with CUDA)
    std::string result =
        executeCommand("nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null");
    return !result.empty();
}

MPSBackend::MPSBackend()
    : available_(false), initialized_(false), running_(false), ownsServer_(false) {
    available_ = checkAvailable();
}

MPSBackend::~MPSBackend() { shutdown(); }

bool MPSBackend::initialize() {
    if (initialized_) {
        return true;
    }

    if (!available_) {
        setError("NVIDIA GPU not available");
        return false;
    }

    // Initialize lock file
    lockFilePath_ = platform::Platform::getInstance()->getTempPath() + "chronos_locks/";
    lockFile_ = std::make_unique<utils::LockFile>(lockFilePath_);
    lockFile_->initializeLockDirectory();

    // Query NVIDIA devices
    if (!queryNvidiaDevices()) {
        setError("Failed to query NVIDIA devices");
        return false;
    }

    if (devices_.empty()) {
        setError("No NVIDIA GPUs found");
        return false;
    }

    // Try to ensure MPS server is running (optional - works without it too)
    // MPS provides better concurrent performance but isn't strictly required
    ensureMPSServerRunning();

    // Start monitor thread
    running_ = true;
    monitorThread_ = std::thread(&MPSBackend::monitorPartitions, this);

    initialized_ = true;
    return true;
}

void MPSBackend::shutdown() {
    if (!initialized_) {
        return;
    }

    // Stop monitor thread
    running_ = false;
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }

    // Release all partitions
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& partition : partitions_) {
            if (partition.info.active) {
                releasePartitionResources(partition);
            }
        }
        partitions_.clear();
    }

    // Stop MPS server if we started it
    if (ownsServer_) {
        stopMPSServer();
    }

    initialized_ = false;
}

bool MPSBackend::queryNvidiaDevices() {
    // Query GPU info using nvidia-smi
    std::string result = executeCommand(
        "nvidia-smi --query-gpu=index,name,memory.total,memory.free,driver_version "
        "--format=csv,noheader,nounits 2>/dev/null");

    if (result.empty()) {
        return false;
    }

    devices_.clear();
    std::istringstream stream(result);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Parse CSV: index, name, memory.total, memory.free, driver_version
        std::istringstream lineStream(line);
        std::string indexStr, name, totalMemStr, freeMemStr, driverVersion;

        std::getline(lineStream, indexStr, ',');
        std::getline(lineStream, name, ',');
        std::getline(lineStream, totalMemStr, ',');
        std::getline(lineStream, freeMemStr, ',');
        std::getline(lineStream, driverVersion);

        // Trim whitespace
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\n\r"));
            s.erase(s.find_last_not_of(" \t\n\r") + 1);
        };

        trim(indexStr);
        trim(name);
        trim(totalMemStr);
        trim(freeMemStr);
        trim(driverVersion);

        BackendDeviceInfo device;
        device.index = std::stoi(indexStr);
        device.name = name;
        device.vendor = "NVIDIA";
        device.driverVersion = driverVersion;
        device.totalMemory =
            static_cast<uint64_t>(std::stod(totalMemStr)) * 1024 * 1024;  // MB to bytes
        device.availableMemory = static_cast<uint64_t>(std::stod(freeMemStr)) * 1024 * 1024;
        device.deviceType = "GPU";
        device.supportsCompute = true;

        devices_.push_back(device);
    }

    return !devices_.empty();
}

bool MPSBackend::ensureMPSServerRunning() {
    // Check if MPS is already running
    std::string status =
        executeCommand("echo get_server_list | nvidia-cuda-mps-control 2>/dev/null");
    if (!status.empty() && status.find("no MPS") == std::string::npos) {
        // MPS is already running
        return true;
    }

    // MPS is not required for basic functionality
    // It's an optional enhancement for better concurrent performance
    // Don't fail if we can't start it
    return false;
}

void MPSBackend::stopMPSServer() {
    if (ownsServer_) {
        system("echo quit | nvidia-cuda-mps-control 2>/dev/null");
        ownsServer_ = false;
    }
}

std::string MPSBackend::generatePartitionId() {
    static std::atomic<int> counter{0};
    int pid = platform::Platform::getInstance()->getProcessId();
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    std::stringstream ss;
    ss << "mps_partition_" << pid << "_" << timestamp << "_"
       << std::setfill('0') << std::setw(4) << ++counter;
    return ss.str();
}

void MPSBackend::monitorPartitions() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto& partition : partitions_) {
                if (partition.info.active && partition.info.isExpired()) {
                    releasePartitionResources(partition);
                    partition.info.active = false;
                }
            }

            // Remove expired partitions
            auto it = std::remove_if(partitions_.begin(), partitions_.end(),
                                     [](const MPSPartition& p) { return !p.info.active; });
            partitions_.erase(it, partitions_.end());
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void MPSBackend::releasePartitionResources(MPSPartition& partition) {
    int deviceIndex = partition.info.deviceIndex;

    if (deviceIndex >= 0 && deviceIndex < static_cast<int>(devices_.size())) {
        uint64_t freedMemory = static_cast<uint64_t>(devices_[deviceIndex].totalMemory *
                                                     partition.info.memoryFraction);
        devices_[deviceIndex].availableMemory += freedMemory;
        lockFile_->releaseLockById(partition.info.partitionId);
    }
}

std::vector<BackendDeviceInfo> MPSBackend::getDevices() const { return devices_; }

BackendDeviceInfo MPSBackend::getDevice(int deviceIndex) const {
    if (deviceIndex >= 0 && deviceIndex < static_cast<int>(devices_.size())) {
        return devices_[deviceIndex];
    }
    return BackendDeviceInfo();
}

std::string MPSBackend::createPartition(int deviceIndex, float memoryFraction, int durationSeconds,
                                        const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Validate inputs
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices_.size())) {
        setError("Invalid device index: " + std::to_string(deviceIndex));
        return "";
    }

    if (memoryFraction <= 0.0f || memoryFraction > 1.0f) {
        setError("Invalid memory fraction. Must be between 0 and 1.");
        return "";
    }

    if (durationSeconds <= 0) {
        setError("Invalid duration. Must be positive.");
        return "";
    }

    std::string currentUser = platform::Platform::getInstance()->getUsername();
    std::string partitionOwner = username.empty() ? currentUser : username;

    // Check admin privileges for creating partitions for other users
    bool isAdmin = (currentUser == "root");
#ifndef _WIN32
    isAdmin = isAdmin || (geteuid() == 0);
#endif

    if (!username.empty() && username != currentUser && !isAdmin) {
        setError("Permission denied: only administrators can create partitions for other users");
        return "";
    }

    auto& device = devices_[deviceIndex];
    uint64_t requestedMemory = static_cast<uint64_t>(device.totalMemory * memoryFraction);

    if (requestedMemory > device.availableMemory) {
        setError("Not enough available memory on device");
        return "";
    }

    std::string partitionId = generatePartitionId();

    if (!lockFile_->createLockById(partitionId, deviceIndex, memoryFraction, partitionOwner)) {
        setError("Failed to create lock for GPU partition");
        return "";
    }

    device.availableMemory -= requestedMemory;

    MPSPartition partition;
    partition.info.partitionId = partitionId;
    partition.info.deviceIndex = deviceIndex;
    partition.info.memoryFraction = memoryFraction;
    partition.info.duration = std::chrono::seconds(durationSeconds);
    partition.info.startTime = std::chrono::system_clock::now();
    partition.info.active = true;
    partition.info.processId = platform::Platform::getInstance()->getProcessId();
    partition.info.username = partitionOwner;
    partition.threadPercentage = static_cast<int>(memoryFraction * 100);

    // Store MPS-specific metadata
    partition.info.metadata["mps_thread_percentage"] = std::to_string(partition.threadPercentage);
    partition.info.metadata["cuda_visible_devices"] = std::to_string(deviceIndex);

    partitions_.push_back(partition);

    return partitionId;
}

bool MPSBackend::releasePartition(const std::string& partitionId) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string currentUser = platform::Platform::getInstance()->getUsername();

    for (auto& partition : partitions_) {
        if (partition.info.partitionId == partitionId && partition.info.active) {
            // Check if user has permission (owner or admin)
            bool isAdmin = (currentUser == "root");
#ifndef _WIN32
            isAdmin = isAdmin || (geteuid() == 0);
#endif
            if (partition.info.username != currentUser && !isAdmin) {
                setError("Permission denied: partition owned by " + partition.info.username);
                return false;
            }

            releasePartitionResources(partition);
            partition.info.active = false;
            return true;
        }
    }

    setError("Partition not found or already released: " + partitionId);
    return false;
}

std::vector<BackendPartition> MPSBackend::listPartitions() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<BackendPartition> result;
    for (const auto& partition : partitions_) {
        if (partition.info.active) {
            result.push_back(partition.info);
        }
    }
    return result;
}

float MPSBackend::getAvailablePercentage(int deviceIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices_.size())) {
        return -1.0f;
    }

    const auto& device = devices_[deviceIndex];
    return 100.0f * (static_cast<float>(device.availableMemory) / device.totalMemory);
}

}  // namespace backends
}  // namespace chronos
