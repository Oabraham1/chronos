/**
 * @file gpu_partition.h
 * @brief GPU partition data structures
 *
 * This file defines the GPUPartition class, which is a fundamental
 * data structure in the Chronos system. It encapsulates all the necessary
 * information for a single GPU partition, including the device ID,
 * memory fraction, duration, and start time.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#ifndef CHRONOS_GPU_PARTITION_H
#define CHRONOS_GPU_PARTITION_H

#include "platform/opencl_include.h"
#include <chrono>
#include <string>

namespace chronos {
namespace core {

/**
 * @class GPUPartition
 * @brief Represents a GPU partition allocation
 */
class GPUPartition {
public:
  /**
   * @brief Default constructor
   */
  GPUPartition();

  /**
   * @brief Check if the partition has expired
   * @return True if expired, false otherwise
   */
  bool isExpired() const;

  /**
   * @brief Get remaining time in seconds
   * @return Seconds remaining until expiration
   */
  int getRemainingTime() const;

  cl_device_id deviceId;
  float memoryFraction;
  std::chrono::seconds duration;
  std::chrono::system_clock::time_point startTime;
  bool active;
  std::string partitionId;
  int processId;
  std::string username;
};

} // namespace core
} // namespace chronos

#endif