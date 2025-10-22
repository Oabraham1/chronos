/**
 * @file memory_enforcer.cpp
 * @brief Description needed
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

#include "core/memory_enforcer.h"

#include <iostream>
#include <map>
#include <mutex>

namespace chronos {
namespace core {

struct BufferInfo {
    cl_mem buffer;
    size_t size;
};

struct PartitionMemory {
    size_t limit;
    size_t currentUsage;
    std::vector<BufferInfo> buffers;
};

class MemoryEnforcer::Impl {
   public:
    cl_device_id deviceId;
    cl_context context;
    std::map<std::string, PartitionMemory> partitions;
    mutable std::mutex mutex;

    Impl(cl_device_id device, cl_context ctx) : deviceId(device), context(ctx) {}

    ~Impl() {
        std::lock_guard<std::mutex> lock(mutex);
        for (std::map<std::string, PartitionMemory>::iterator it = partitions.begin();
             it != partitions.end(); ++it) {
            for (size_t i = 0; i < it->second.buffers.size(); ++i) {
                if (it->second.buffers[i].buffer) {
                    clReleaseMemObject(it->second.buffers[i].buffer);
                }
            }
        }
    }
};

MemoryEnforcer::MemoryEnforcer(cl_device_id deviceId, cl_context context)
    : pImpl(std::make_unique<Impl>(deviceId, context)) {}

MemoryEnforcer::~MemoryEnforcer() = default;

bool MemoryEnforcer::allocatePartition(const std::string& partitionId, size_t memoryLimit) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);

    if (pImpl->partitions.find(partitionId) != pImpl->partitions.end()) {
        std::cerr << "Partition " << partitionId << " already exists" << std::endl;
        return false;
    }

    PartitionMemory memory;
    memory.limit = memoryLimit;
    memory.currentUsage = 0;

    pImpl->partitions[partitionId] = memory;

    std::cout << "Allocated partition " << partitionId << " with limit "
              << (memoryLimit / (1024 * 1024)) << " MB" << std::endl;

    return true;
}

bool MemoryEnforcer::canAllocate(const std::string& partitionId, size_t requestedBytes) const {
    std::lock_guard<std::mutex> lock(pImpl->mutex);

    std::map<std::string, PartitionMemory>::const_iterator it = pImpl->partitions.find(partitionId);
    if (it == pImpl->partitions.end()) {
        return false;
    }

    const PartitionMemory& memory = it->second;
    return (memory.currentUsage + requestedBytes) <= memory.limit;
}

bool MemoryEnforcer::trackBuffer(const std::string& partitionId, cl_mem buffer, size_t size) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);

    std::map<std::string, PartitionMemory>::iterator it = pImpl->partitions.find(partitionId);
    if (it == pImpl->partitions.end()) {
        std::cerr << "Partition " << partitionId << " not found" << std::endl;
        return false;
    }

    PartitionMemory& memory = it->second;

    if (memory.currentUsage + size > memory.limit) {
        std::cerr << "Memory allocation would exceed partition limit" << std::endl;
        std::cerr << "Current: " << (memory.currentUsage / (1024 * 1024)) << " MB, "
                  << "Requested: " << (size / (1024 * 1024)) << " MB, "
                  << "Limit: " << (memory.limit / (1024 * 1024)) << " MB" << std::endl;
        return false;
    }

    BufferInfo bufferInfo;
    bufferInfo.buffer = buffer;
    bufferInfo.size = size;

    memory.buffers.push_back(bufferInfo);
    memory.currentUsage += size;

    return true;
}

bool MemoryEnforcer::releaseBuffer(const std::string& partitionId, cl_mem buffer) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);

    std::map<std::string, PartitionMemory>::iterator it = pImpl->partitions.find(partitionId);
    if (it == pImpl->partitions.end()) {
        return false;
    }

    PartitionMemory& memory = it->second;

    for (std::vector<BufferInfo>::iterator bufIt = memory.buffers.begin();
         bufIt != memory.buffers.end(); ++bufIt) {
        if (bufIt->buffer == buffer) {
            size_t size = bufIt->size;
            memory.currentUsage -= size;
            memory.buffers.erase(bufIt);
            clReleaseMemObject(buffer);
            return true;
        }
    }

    return false;
}

size_t MemoryEnforcer::getCurrentUsage(const std::string& partitionId) const {
    std::lock_guard<std::mutex> lock(pImpl->mutex);

    std::map<std::string, PartitionMemory>::const_iterator it = pImpl->partitions.find(partitionId);
    if (it == pImpl->partitions.end()) {
        return 0;
    }

    return it->second.currentUsage;
}

size_t MemoryEnforcer::getMemoryLimit(const std::string& partitionId) const {
    std::lock_guard<std::mutex> lock(pImpl->mutex);

    std::map<std::string, PartitionMemory>::const_iterator it = pImpl->partitions.find(partitionId);
    if (it == pImpl->partitions.end()) {
        return 0;
    }

    return it->second.limit;
}

void MemoryEnforcer::releasePartition(const std::string& partitionId) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);

    std::map<std::string, PartitionMemory>::iterator it = pImpl->partitions.find(partitionId);
    if (it == pImpl->partitions.end()) {
        return;
    }

    PartitionMemory& memory = it->second;

    for (size_t i = 0; i < memory.buffers.size(); ++i) {
        if (memory.buffers[i].buffer) {
            clReleaseMemObject(memory.buffers[i].buffer);
        }
    }

    pImpl->partitions.erase(it);
}

std::vector<std::string> MemoryEnforcer::getActivePartitions() const {
    std::lock_guard<std::mutex> lock(pImpl->mutex);

    std::vector<std::string> result;
    for (std::map<std::string, PartitionMemory>::const_iterator it = pImpl->partitions.begin();
         it != pImpl->partitions.end(); ++it) {
        result.push_back(it->first);
    }

    return result;
}

}  // namespace core
}  // namespace chronos
