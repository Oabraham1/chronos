/**
 * @file partitioner.cpp
 * @brief Implementation of the ChronosPartitioner class
 *
 * This file contains the core implementation of the ChronosPartitioner class,
 * which is responsible for managing the lifecycle of GPU partitions. It uses
 * a Pimpl idiom to hide the implementation details and provide a clean
 * public API.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#include "chronos.h"
#include "core/device_info.h"
#include "core/gpu_partition.h"
#include "platform/opencl_include.h"
#include "platform/platform.h"
#include "utils/lock_file.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

namespace chronos {

class ChronosPartitioner::Impl {
public:
  Impl();
  ~Impl();

  void initializeDevices();
  std::string generatePartitionId();
  void monitorPartitions();
  void releasePartitionResources(core::GPUPartition &partition);
  int getDeviceIndex(cl_device_id deviceId);
  bool canAccessGPU(int deviceIdx, float memoryFraction);

  std::string createPartition(int deviceIdx, float memoryFraction,
                              int durationInSeconds);
  std::vector<core::GPUPartition> listPartitions(bool printOutput);
  bool releasePartition(const std::string &partitionId);
  void showDeviceStats();
  float getGPUAvailablePercentage(int deviceIdx);

private:
  cl_platform_id platform;
  cl_context context;

  std::string lockFilePath;
  utils::LockFile lockFile;

  std::vector<core::DeviceInfo> devices;
  std::vector<core::GPUPartition> partitions;
  std::mutex partitionMutex;

  bool running;
  std::thread monitorThread;
};

ChronosPartitioner::ChronosPartitioner() : pImpl(std::make_unique<Impl>()) {}

ChronosPartitioner::~ChronosPartitioner() = default;

std::string ChronosPartitioner::createPartition(int deviceIdx,
                                                float memoryFraction,
                                                int durationInSeconds) {
  return pImpl->createPartition(deviceIdx, memoryFraction, durationInSeconds);
}

std::vector<ChronosPartitioner::GPUPartition>
ChronosPartitioner::listPartitions(bool printOutput) {
  return pImpl->listPartitions(printOutput);
}

bool ChronosPartitioner::releasePartition(const std::string &partitionId) {
  return pImpl->releasePartition(partitionId);
}

void ChronosPartitioner::showDeviceStats() { pImpl->showDeviceStats(); }

float ChronosPartitioner::getGPUAvailablePercentage(int deviceIdx) {
  return pImpl->getGPUAvailablePercentage(deviceIdx);
}

ChronosPartitioner::Impl::Impl()
    : platform(nullptr), context(nullptr),
      lockFilePath(platform::Platform::getInstance()->getTempPath() +
                   "chronos_locks/"),
      lockFile(lockFilePath), running(true) {
  lockFile.initializeLockDirectory();

  initializeDevices();

  monitorThread =
      std::thread(&ChronosPartitioner::Impl::monitorPartitions, this);
}

ChronosPartitioner::Impl::~Impl() {
  running = false;
  if (monitorThread.joinable()) {
    monitorThread.join();
  }

  std::lock_guard<std::mutex> lock(partitionMutex);
  for (auto &partition : partitions) {
    if (partition.active) {
      releasePartitionResources(partition);
    }
  }
  partitions.clear();

  if (context) {
    clReleaseContext(context);
  }
}

void ChronosPartitioner::Impl::initializeDevices() {
  cl_int err;
  cl_uint numPlatforms;

  err = clGetPlatformIDs(0, nullptr, &numPlatforms);
  if (err != CL_SUCCESS || numPlatforms == 0) {
    std::cerr << "No OpenCL platforms found" << std::endl;
    return;
  }

  std::vector<cl_platform_id> platforms(numPlatforms);
  err = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
  if (err != CL_SUCCESS) {
    std::cerr << "Failed to get OpenCL platform IDs" << std::endl;
    return;
  }

  platform = platforms[0];

  cl_uint numDevices;
  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
  if (err != CL_SUCCESS || numDevices == 0) {
    std::cerr << "No OpenCL devices found" << std::endl;
    return;
  }

  std::vector<cl_device_id> deviceIds(numDevices);
  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices,
                       deviceIds.data(), nullptr);
  if (err != CL_SUCCESS) {
    std::cerr << "Failed to get OpenCL device IDs" << std::endl;
    return;
  }

  cl_context_properties props[] = {CL_CONTEXT_PLATFORM,
                                   (cl_context_properties)platform, 0};

  context = clCreateContext(props, numDevices, deviceIds.data(), nullptr,
                            nullptr, &err);
  if (err != CL_SUCCESS) {
    std::cerr << "Failed to create OpenCL context" << std::endl;
    return;
  }

  std::cout << "Found " << numDevices << " OpenCL device(s)" << std::endl;

  for (cl_uint i = 0; i < numDevices; i++) {
    core::DeviceInfo device(deviceIds[i]);
    if (device.loadDeviceInfo()) {
      devices.push_back(device);

      std::cout << "Device " << i << ": " << device.name << std::endl;
      std::cout << "  Type: " << device.getDeviceTypeString() << std::endl;
      std::cout << "  Vendor: " << device.vendor << std::endl;
      std::cout << "  OpenCL version: " << device.version << std::endl;
      std::cout << "  Total memory: " << (device.totalMemory / (1024 * 1024))
                << " MB" << std::endl;
    }
  }
}

std::string ChronosPartitioner::Impl::generatePartitionId() {
  static int counter = 0;
  std::stringstream ss;
  ss << "partition_" << std::setfill('0') << std::setw(4) << ++counter;
  return ss.str();
}

void ChronosPartitioner::Impl::monitorPartitions() {
  while (running) {
    {
      std::lock_guard<std::mutex> lock(partitionMutex);
      auto now = std::chrono::system_clock::now();

      for (auto &partition : partitions) {
        if (partition.active) {
          auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
              now - partition.startTime);

          if (elapsed >= partition.duration) {
            releasePartitionResources(partition);
            partition.active = false;
            std::cout << "Partition " << partition.partitionId
                      << " expired and released" << std::endl;
          }
        }
      }

      auto it =
          std::remove_if(partitions.begin(), partitions.end(),
                         [](const core::GPUPartition &p) { return !p.active; });
      partitions.erase(it, partitions.end());
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void ChronosPartitioner::Impl::releasePartitionResources(
    core::GPUPartition &partition) {
  for (auto &device : devices) {
    if (device.id == partition.deviceId) {
      cl_ulong freedMemory = device.totalMemory * partition.memoryFraction;
      device.availableMemory += freedMemory;

      lockFile.releaseLock(getDeviceIndex(partition.deviceId),
                           partition.memoryFraction);
      break;
    }
  }
}

int ChronosPartitioner::Impl::getDeviceIndex(cl_device_id deviceId) {
  for (size_t i = 0; i < devices.size(); i++) {
    if (devices[i].id == deviceId) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool ChronosPartitioner::Impl::canAccessGPU(int deviceIdx,
                                            float memoryFraction) {
  if (deviceIdx < 0 || static_cast<size_t>(deviceIdx) >= devices.size()) {
    std::cerr << "Invalid device index: " << deviceIdx << std::endl;
    return false;
  }

  cl_ulong requestedMemory = devices[deviceIdx].totalMemory * memoryFraction;

  if (requestedMemory > devices[deviceIdx].availableMemory) {
    std::cerr << "Not enough available memory on device " << deviceIdx
              << std::endl;
    std::cerr << "Requested: " << (requestedMemory / (1024 * 1024)) << " MB, "
              << "Available: "
              << (devices[deviceIdx].availableMemory / (1024 * 1024)) << " MB"
              << std::endl;
    return false;
  }

  if (lockFile.lockExists(deviceIdx, memoryFraction)) {
    std::string owner = lockFile.getLockOwner(deviceIdx, memoryFraction);
    std::string currentUser = platform::Platform::getInstance()->getUsername();

    if (owner != currentUser) {
      std::cerr << "GPU partition is locked by user: " << owner << std::endl;
      return false;
    }
  }

  return true;
}

std::string ChronosPartitioner::Impl::createPartition(int deviceIdx,
                                                      float memoryFraction,
                                                      int durationInSeconds) {
  std::lock_guard<std::mutex> lock(partitionMutex);

  if (deviceIdx < 0 || static_cast<size_t>(deviceIdx) >= devices.size()) {
    std::cerr << "Invalid device index: " << deviceIdx << std::endl;
    return "";
  }

  if (memoryFraction <= 0.0f || memoryFraction > 1.0f) {
    std::cerr << "Invalid memory fraction. Must be between 0 and 1."
              << std::endl;
    return "";
  }

  if (durationInSeconds <= 0) {
    std::cerr << "Invalid duration. Must be positive." << std::endl;
    return "";
  }

  if (!canAccessGPU(deviceIdx, memoryFraction)) {
    std::cerr
        << "Cannot create partition: GPU portion is locked by another user"
        << std::endl;
    return "";
  }

  core::DeviceInfo &device = devices[deviceIdx];
  cl_ulong requestedMemory = device.totalMemory * memoryFraction;

  if (requestedMemory > device.availableMemory) {
    std::cerr << "Not enough available memory on device " << deviceIdx
              << std::endl;
    std::cerr << "Requested: " << (requestedMemory / (1024 * 1024)) << " MB, "
              << "Available: " << (device.availableMemory / (1024 * 1024))
              << " MB" << std::endl;
    return "";
  }

  std::string partitionId = generatePartitionId();

  if (!lockFile.createLock(deviceIdx, memoryFraction, partitionId)) {
    std::cerr << "Failed to create lock for GPU partition" << std::endl;
    return "";
  }

  device.availableMemory -= requestedMemory;

  core::GPUPartition partition;
  partition.deviceId = device.id;
  partition.memoryFraction = memoryFraction;
  partition.duration = std::chrono::seconds(durationInSeconds);
  partition.startTime = std::chrono::system_clock::now();
  partition.active = true;
  partition.partitionId = partitionId;
  partition.processId = platform::Platform::getInstance()->getProcessId();
  partition.username = platform::Platform::getInstance()->getUsername();

  partitions.push_back(partition);

  std::cout << "Created partition " << partition.partitionId << " on device "
            << deviceIdx << " (" << device.name << ") with "
            << (requestedMemory / (1024 * 1024)) << " MB for "
            << durationInSeconds << " seconds" << std::endl;
  std::cout << "Locked for exclusive use by " << partition.username
            << " (PID: " << partition.processId << ")" << std::endl;

  return partition.partitionId;
}

std::vector<core::GPUPartition>
ChronosPartitioner::Impl::listPartitions(bool printOutput) {
  std::lock_guard<std::mutex> lock(partitionMutex);

  std::vector<core::GPUPartition> activePartitions;

  for (const auto &partition : partitions) {
    if (partition.active) {
      activePartitions.push_back(partition);
    }
  }

  if (printOutput) {
    if (activePartitions.empty()) {
      std::cout << "No active partitions" << std::endl;
      return activePartitions;
    }

    std::cout << "Active partitions:" << std::endl;
    std::cout << "-----------------" << std::endl;

    for (const auto &partition : activePartitions) {
      auto now = std::chrono::system_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          now - partition.startTime);
      auto remaining = partition.duration - elapsed;

      int deviceIdx = getDeviceIndex(partition.deviceId);
      std::string deviceName =
          (deviceIdx >= 0) ? devices[deviceIdx].name : "Unknown";

      std::cout << "ID: " << partition.partitionId << std::endl;
      std::cout << "  Device: " << deviceIdx << " (" << deviceName << ")"
                << std::endl;
      std::cout << "  Memory: " << (partition.memoryFraction * 100) << "%"
                << std::endl;
      std::cout << "  Time remaining: " << remaining.count() << " seconds"
                << std::endl;
      std::cout << "  Owner: " << partition.username
                << " (PID: " << partition.processId << ")" << std::endl;
      std::cout << std::endl;
    }
  }

  return activePartitions;
}

bool ChronosPartitioner::Impl::releasePartition(
    const std::string &partitionId) {
  std::lock_guard<std::mutex> lock(partitionMutex);

  std::string currentUser = platform::Platform::getInstance()->getUsername();

  for (auto &partition : partitions) {
    if (partition.partitionId == partitionId && partition.active) {
      if (partition.username != currentUser) {
        std::cerr << "Permission denied: partition owned by "
                  << partition.username << std::endl;
        return false;
      }

      releasePartitionResources(partition);
      partition.active = false;
      std::cout << "Partition " << partitionId << " released" << std::endl;
      return true;
    }
  }

  std::cerr << "Partition not found or already released: " << partitionId
            << std::endl;
  return false;
}

void ChronosPartitioner::Impl::showDeviceStats() {
  std::lock_guard<std::mutex> lock(partitionMutex);

  std::cout << "Device statistics:" << std::endl;
  std::cout << "=================" << std::endl;

  for (size_t i = 0; i < devices.size(); i++) {
    const auto &device = devices[i];

    float memoryUsagePercent =
        100.0f * (1.0f - (float)device.availableMemory / device.totalMemory);

    std::cout << "Device " << i << ": " << device.name << std::endl;
    std::cout << "  Type: " << device.getDeviceTypeString() << std::endl;
    std::cout << "  Vendor: " << device.vendor << std::endl;
    std::cout << "  OpenCL version: " << device.version << std::endl;
    std::cout << "  Memory:" << std::endl;
    std::cout << "    Total: " << (device.totalMemory / (1024 * 1024)) << " MB"
              << std::endl;
    std::cout << "    Used: "
              << ((device.totalMemory - device.availableMemory) / (1024 * 1024))
              << " MB" << std::endl;
    std::cout << "    Available: " << (device.availableMemory / (1024 * 1024))
              << " MB" << std::endl;
    std::cout << "    Usage: " << std::fixed << std::setprecision(2)
              << memoryUsagePercent << "%" << std::endl;

    int activePartitions = 0;
    for (const auto &partition : partitions) {
      if (partition.deviceId == device.id && partition.active) {
        activePartitions++;
      }
    }
    std::cout << "  Chronos management:" << std::endl;
    std::cout << "    Active partitions: " << activePartitions << std::endl;

    std::cout << std::endl;
  }
}

float ChronosPartitioner::Impl::getGPUAvailablePercentage(int deviceIdx) {
  std::lock_guard<std::mutex> lock(partitionMutex);

  if (deviceIdx < 0 || static_cast<size_t>(deviceIdx) >= devices.size()) {
    std::cerr << "Invalid device index: " << deviceIdx << std::endl;
    return -1.0f;
  }

  const auto &device = devices[deviceIdx];
  return 100.0f * ((float)device.availableMemory / device.totalMemory);
}

} // namespace chronos