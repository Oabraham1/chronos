/**
 * @file gpu_partition.cpp
 * @brief Implementation of the GPUPartition class.
 *
 * This file provides the implementation for the GPUPartition class, which
 * represents a single GPU partition in the Chronos system. It includes
 * functions for checking if a partition has expired and for getting the
 * remaining time until expiration.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#include "core/gpu_partition.h"

namespace chronos {
namespace core {

GPUPartition::GPUPartition()
    : deviceId(nullptr),
      memoryFraction(0.0f),
      duration(std::chrono::seconds(0)),
      startTime(std::chrono::system_clock::now()),
      active(false),
      processId(0) {}

bool GPUPartition::isExpired() const {
    if (!active) {
        return true;
    }

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

    return elapsed >= duration;
}

int GPUPartition::getRemainingTime() const {
    if (!active) {
        return 0;
    }

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

    if (elapsed >= duration) {
        return 0;
    }

    return static_cast<int>((duration - elapsed).count());
}

}  // namespace core
}  // namespace chronos