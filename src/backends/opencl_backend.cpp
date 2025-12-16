/**
 * @file opencl_backend.cpp
 * @brief OpenCL execution backend implementation
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

#include "backends/opencl_backend.h"

#include <algorithm>
#include <atomic>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "platform/platform.h"

namespace chronos {
namespace backends {

bool OpenCLBackend::checkAvailable() {
    cl_uint numPlatforms = 0;
    cl_int err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    return (err == CL_SUCCESS && numPlatforms > 0);
}

OpenCLBackend::OpenCLBackend()
    : available_(false),
      initialized_(false),
      running_(false),
      platform_(nullptr),
      context_(nullptr) {
    available_ = checkAvailable();
}

OpenCLBackend::~OpenCLBackend() { shutdown(); }

bool OpenCLBackend::initialize() {
    if (initialized_) {
        return true;
    }

    if (!available_) {
        setError("OpenCL is not available on this system");
        return false;
    }

    // Initialize lock file
    lockFilePath_ = platform::Platform::getInstance()->getTempPath() + "chronos_locks/";
    lockFile_ = std::make_unique<utils::LockFile>(lockFilePath_);
    lockFile_->initializeLockDirectory();

    // Initialize devices
    initializeDevices();

    if (devices_.empty()) {
        setError("No OpenCL devices found");
        return false;
    }

    // Initialize memory enforcers
    initializeEnforcers();

    // Start monitor thread
    running_ = true;
    monitorThread_ = std::thread(&OpenCLBackend::monitorPartitions, this);

    initialized_ = true;
    return true;
}

void OpenCLBackend::shutdown() {
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

    // Release OpenCL context
    if (context_) {
        clReleaseContext(context_);
        context_ = nullptr;
    }

    // Clear enforcers
    enforcers_.clear();

    initialized_ = false;
}

void OpenCLBackend::initializeDevices() {
    cl_int err;
    cl_uint numPlatforms;

    err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        setError("No OpenCL platforms found");
        return;
    }

    std::vector<cl_platform_id> platforms(numPlatforms);
    err = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
    if (err != CL_SUCCESS) {
        setError("Failed to get OpenCL platform IDs");
        return;
    }

    platform_ = platforms[0];

    cl_uint numDevices;
    err = clGetDeviceIDs(platform_, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    if (err != CL_SUCCESS || numDevices == 0) {
        setError("No OpenCL devices found");
        return;
    }

    std::vector<cl_device_id> deviceIds(numDevices);
    err = clGetDeviceIDs(platform_, CL_DEVICE_TYPE_ALL, numDevices, deviceIds.data(), nullptr);
    if (err != CL_SUCCESS) {
        setError("Failed to get OpenCL device IDs");
        return;
    }

    cl_context_properties props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform_, 0};

    context_ = clCreateContext(props, numDevices, deviceIds.data(), nullptr, nullptr, &err);
    if (err != CL_SUCCESS) {
        setError("Failed to create OpenCL context");
        return;
    }

    for (cl_uint i = 0; i < numDevices; i++) {
        core::DeviceInfo coreDevice(deviceIds[i]);
        if (!coreDevice.loadDeviceInfo()) {
            continue;
        }

        InternalDevice device;
        device.clDeviceId = deviceIds[i];
        device.info.index = static_cast<int>(i);
        device.info.name = coreDevice.name;
        device.info.vendor = coreDevice.vendor;
        device.info.driverVersion = coreDevice.version;
        device.info.totalMemory = coreDevice.totalMemory;
        device.info.availableMemory = coreDevice.availableMemory;
        device.info.deviceType = coreDevice.getDeviceTypeString();
        device.info.supportsCompute = true;
        device.allocatedMemory = 0;

        devices_.push_back(device);
    }
}

void OpenCLBackend::initializeEnforcers() {
    for (size_t i = 0; i < devices_.size(); i++) {
        enforcers_[i] = std::make_unique<core::MemoryEnforcer>(devices_[i].clDeviceId, context_);
    }
}

std::string OpenCLBackend::generatePartitionId() {
    static std::atomic<int> counter{0};
    int pid = platform::Platform::getInstance()->getProcessId();
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    std::stringstream ss;
    ss << "partition_" << pid << "_" << timestamp << "_"
       << std::setfill('0') << std::setw(4) << ++counter;
    return ss.str();
}

void OpenCLBackend::monitorPartitions() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto now = std::chrono::system_clock::now();

            for (auto& partition : partitions_) {
                if (partition.info.active && partition.info.isExpired()) {
                    releasePartitionResources(partition);
                    partition.info.active = false;
                }
            }

            // Remove expired partitions
            auto it = std::remove_if(partitions_.begin(), partitions_.end(),
                                     [](const InternalPartition& p) { return !p.info.active; });
            partitions_.erase(it, partitions_.end());
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void OpenCLBackend::releasePartitionResources(InternalPartition& partition) {
    int deviceIndex = getDeviceIndexByClId(partition.clDeviceId);

    if (deviceIndex >= 0 && enforcers_.find(deviceIndex) != enforcers_.end()) {
        enforcers_[deviceIndex]->releasePartition(partition.info.partitionId);
    }

    if (deviceIndex >= 0 && deviceIndex < static_cast<int>(devices_.size())) {
        uint64_t freedMemory = static_cast<uint64_t>(devices_[deviceIndex].info.totalMemory *
                                                     partition.info.memoryFraction);
        devices_[deviceIndex].allocatedMemory -= freedMemory;
        devices_[deviceIndex].info.availableMemory += freedMemory;
        lockFile_->releaseLockById(partition.info.partitionId);
    }
}

int OpenCLBackend::getDeviceIndexByClId(cl_device_id deviceId) const {
    for (size_t i = 0; i < devices_.size(); i++) {
        if (devices_[i].clDeviceId == deviceId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool OpenCLBackend::canAccessGPU(int deviceIndex, float memoryFraction) const {
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices_.size())) {
        return false;
    }

    const auto& device = devices_[deviceIndex];
    uint64_t requestedMemory = static_cast<uint64_t>(device.info.totalMemory * memoryFraction);

    if (requestedMemory > device.info.availableMemory) {
        return false;
    }

    return true;
}

std::vector<BackendDeviceInfo> OpenCLBackend::getDevices() const {
    std::vector<BackendDeviceInfo> result;
    result.reserve(devices_.size());
    for (const auto& device : devices_) {
        result.push_back(device.info);
    }
    return result;
}

BackendDeviceInfo OpenCLBackend::getDevice(int deviceIndex) const {
    if (deviceIndex >= 0 && deviceIndex < static_cast<int>(devices_.size())) {
        return devices_[deviceIndex].info;
    }
    return BackendDeviceInfo();
}

std::string OpenCLBackend::createPartition(int deviceIndex, float memoryFraction,
                                           int durationSeconds, const std::string& username) {
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
#ifdef _WIN32
    BOOL isElevated = FALSE;
    HANDLE token = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isElevated = elevation.TokenIsElevated;
        }
        CloseHandle(token);
    }
    isAdmin = isElevated;
#else
    isAdmin = isAdmin || (geteuid() == 0);
#endif

    if (!username.empty() && username != currentUser && !isAdmin) {
        setError("Permission denied: only administrators can create partitions for other users");
        return "";
    }

    if (!canAccessGPU(deviceIndex, memoryFraction)) {
        setError("Cannot create partition: GPU portion is locked or not enough memory");
        return "";
    }

    auto& device = devices_[deviceIndex];
    uint64_t requestedMemory = static_cast<uint64_t>(device.info.totalMemory * memoryFraction);

    if (requestedMemory > device.info.availableMemory) {
        setError("Not enough available memory on device");
        return "";
    }

    std::string partitionId = generatePartitionId();

    if (!lockFile_->createLockById(partitionId, deviceIndex, memoryFraction, partitionOwner)) {
        setError("Failed to create lock for GPU partition");
        return "";
    }

    if (enforcers_.find(deviceIndex) != enforcers_.end()) {
        if (!enforcers_[deviceIndex]->allocatePartition(partitionId, requestedMemory)) {
            setError("Failed to allocate memory enforcer for partition");
            lockFile_->releaseLockById(partitionId);
            return "";
        }
    }

    device.info.availableMemory -= requestedMemory;
    device.allocatedMemory += requestedMemory;

    InternalPartition partition;
    partition.clDeviceId = device.clDeviceId;
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

bool OpenCLBackend::releasePartition(const std::string& partitionId) {
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

std::vector<BackendPartition> OpenCLBackend::listPartitions() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<BackendPartition> result;
    for (const auto& partition : partitions_) {
        if (partition.info.active) {
            result.push_back(partition.info);
        }
    }
    return result;
}

float OpenCLBackend::getAvailablePercentage(int deviceIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices_.size())) {
        return -1.0f;
    }

    const auto& device = devices_[deviceIndex];
    return 100.0f * (static_cast<float>(device.info.availableMemory) / device.info.totalMemory);
}

}  // namespace backends
}  // namespace chronos
