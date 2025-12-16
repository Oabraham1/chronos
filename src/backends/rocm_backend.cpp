/**
 * @file rocm_backend.cpp
 * @brief AMD ROCm execution backend implementation
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

#include "backends/rocm_backend.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "platform/platform.h"

namespace chronos {
namespace backends {

namespace {

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

bool commandExists(const std::string& cmd) {
#ifdef _WIN32
    std::string check = "where " + cmd + " >nul 2>&1";
#else
    std::string check = "which " + cmd + " >/dev/null 2>&1";
#endif
    return system(check.c_str()) == 0;
}

}  // anonymous namespace

bool ROCmBackend::checkAvailable() { return commandExists("rocm-smi"); }

ROCmBackend::ROCmBackend() : available_(false), initialized_(false), running_(false) {
    available_ = checkAvailable();
}

ROCmBackend::~ROCmBackend() { shutdown(); }

bool ROCmBackend::initialize() {
    if (initialized_) {
        return true;
    }

    if (!available_) {
        setError("ROCm is not available on this system");
        return false;
    }

    // Initialize lock file
    lockFilePath_ = platform::Platform::getInstance()->getTempPath() + "chronos_locks/";
    lockFile_ = std::make_unique<utils::LockFile>(lockFilePath_);
    lockFile_->initializeLockDirectory();

    // Query ROCm devices
    if (!queryROCmDevices()) {
        setError("Failed to query ROCm devices");
        return false;
    }

    if (devices_.empty()) {
        setError("No AMD GPUs found");
        return false;
    }

    // Start monitor thread
    running_ = true;
    monitorThread_ = std::thread(&ROCmBackend::monitorPartitions, this);

    initialized_ = true;
    return true;
}

void ROCmBackend::shutdown() {
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

    initialized_ = false;
}

bool ROCmBackend::queryROCmDevices() {
    // Query GPU info using rocm-smi
    std::string result =
        executeCommand("rocm-smi --showid --showproductname --showmeminfo vram 2>/dev/null");

    if (result.empty()) {
        return false;
    }

    devices_.clear();

    // Parse rocm-smi output
    // This is a simplified parser - rocm-smi output format may vary
    std::istringstream stream(result);
    std::string line;
    int currentGpuIndex = -1;
    BackendDeviceInfo currentDevice;
    bool inDevice = false;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Look for GPU index patterns
        if (line.find("GPU[") != std::string::npos) {
            if (inDevice && currentGpuIndex >= 0) {
                devices_.push_back(currentDevice);
            }

            // Extract GPU index
            size_t start = line.find('[') + 1;
            size_t end = line.find(']');
            if (start != std::string::npos && end != std::string::npos) {
                currentGpuIndex = std::stoi(line.substr(start, end - start));
                currentDevice = BackendDeviceInfo();
                currentDevice.index = currentGpuIndex;
                currentDevice.vendor = "AMD";
                currentDevice.deviceType = "GPU";
                currentDevice.supportsCompute = true;
                inDevice = true;
            }
        }

        // Look for product name
        if (line.find("Product Name") != std::string::npos ||
            line.find("Card series") != std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                currentDevice.name = line.substr(colonPos + 1);
                // Trim whitespace
                currentDevice.name.erase(0, currentDevice.name.find_first_not_of(" \t"));
                currentDevice.name.erase(currentDevice.name.find_last_not_of(" \t\n\r") + 1);
            }
        }

        // Look for VRAM info
        if (line.find("VRAM Total") != std::string::npos ||
            line.find("Total Memory") != std::string::npos) {
            // Extract memory value (usually in MB or GB)
            std::regex memRegex("(\\d+)\\s*(MB|GB|MiB|GiB)");
            std::smatch match;
            if (std::regex_search(line, match, memRegex)) {
                uint64_t memValue = std::stoull(match[1].str());
                std::string unit = match[2].str();
                if (unit == "GB" || unit == "GiB") {
                    memValue *= 1024 * 1024 * 1024;
                } else {
                    memValue *= 1024 * 1024;
                }
                currentDevice.totalMemory = memValue;
                currentDevice.availableMemory = memValue;  // Assume all available initially
            }
        }
    }

    // Add last device
    if (inDevice && currentGpuIndex >= 0) {
        devices_.push_back(currentDevice);
    }

    // If parsing failed, try a simpler approach
    if (devices_.empty()) {
        // Try to at least detect GPU count
        result = executeCommand("rocm-smi --showid 2>/dev/null | grep -c GPU");
        if (!result.empty()) {
            int gpuCount = std::stoi(result);
            for (int i = 0; i < gpuCount; i++) {
                BackendDeviceInfo device;
                device.index = i;
                device.name = "AMD GPU " + std::to_string(i);
                device.vendor = "AMD";
                device.deviceType = "GPU";
                device.supportsCompute = true;
                device.totalMemory = 8ULL * 1024 * 1024 * 1024;  // Assume 8GB
                device.availableMemory = device.totalMemory;
                devices_.push_back(device);
            }
        }
    }

    // Get driver version
    std::string driverResult = executeCommand("rocm-smi --showdriverversion 2>/dev/null");
    if (!driverResult.empty()) {
        for (auto& device : devices_) {
            size_t colonPos = driverResult.find(':');
            if (colonPos != std::string::npos) {
                device.driverVersion = driverResult.substr(colonPos + 1);
                device.driverVersion.erase(0, device.driverVersion.find_first_not_of(" \t"));
                device.driverVersion.erase(device.driverVersion.find_last_not_of(" \t\n\r") + 1);
            }
        }
    }

    return !devices_.empty();
}

std::string ROCmBackend::generatePartitionId() {
    static std::atomic<int> counter{0};
    int pid = platform::Platform::getInstance()->getProcessId();
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    std::stringstream ss;
    ss << "rocm_partition_" << pid << "_" << timestamp << "_"
       << std::setfill('0') << std::setw(4) << ++counter;
    return ss.str();
}

void ROCmBackend::monitorPartitions() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto& partition : partitions_) {
                if (partition.info.active && partition.info.isExpired()) {
                    releasePartitionResources(partition);
                    partition.info.active = false;
                }
            }

            auto it = std::remove_if(partitions_.begin(), partitions_.end(),
                                     [](const ROCmPartition& p) { return !p.info.active; });
            partitions_.erase(it, partitions_.end());
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void ROCmBackend::releasePartitionResources(ROCmPartition& partition) {
    int deviceIndex = partition.info.deviceIndex;

    if (deviceIndex >= 0 && deviceIndex < static_cast<int>(devices_.size())) {
        uint64_t freedMemory = static_cast<uint64_t>(devices_[deviceIndex].totalMemory *
                                                     partition.info.memoryFraction);
        devices_[deviceIndex].availableMemory += freedMemory;
        lockFile_->releaseLockById(partition.info.partitionId);
    }
}

std::vector<BackendDeviceInfo> ROCmBackend::getDevices() const { return devices_; }

BackendDeviceInfo ROCmBackend::getDevice(int deviceIndex) const {
    if (deviceIndex >= 0 && deviceIndex < static_cast<int>(devices_.size())) {
        return devices_[deviceIndex];
    }
    return BackendDeviceInfo();
}

std::string ROCmBackend::createPartition(int deviceIndex, float memoryFraction, int durationSeconds,
                                         const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);

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

    ROCmPartition partition;
    partition.info.partitionId = partitionId;
    partition.info.deviceIndex = deviceIndex;
    partition.info.memoryFraction = memoryFraction;
    partition.info.duration = std::chrono::seconds(durationSeconds);
    partition.info.startTime = std::chrono::system_clock::now();
    partition.info.active = true;
    partition.info.processId = platform::Platform::getInstance()->getProcessId();
    partition.info.username = partitionOwner;

    partitions_.push_back(partition);

    return partitionId;
}

bool ROCmBackend::releasePartition(const std::string& partitionId) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string currentUser = platform::Platform::getInstance()->getUsername();

    for (auto& partition : partitions_) {
        if (partition.info.partitionId == partitionId && partition.info.active) {
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

std::vector<BackendPartition> ROCmBackend::listPartitions() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<BackendPartition> result;
    for (const auto& partition : partitions_) {
        if (partition.info.active) {
            result.push_back(partition.info);
        }
    }
    return result;
}

float ROCmBackend::getAvailablePercentage(int deviceIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices_.size())) {
        return -1.0f;
    }

    const auto& device = devices_[deviceIndex];
    return 100.0f * (static_cast<float>(device.availableMemory) / device.totalMemory);
}

}  // namespace backends
}  // namespace chronos
