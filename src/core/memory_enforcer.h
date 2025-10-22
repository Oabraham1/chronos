/**
 * @file memory_enforcer.h
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

#ifndef CHRONOS_MEMORY_ENFORCER_H
#define CHRONOS_MEMORY_ENFORCER_H

#include <memory>
#include <string>
#include <vector>

#include "platform/opencl_include.h"

namespace chronos {
namespace core {

class MemoryEnforcer {
   public:
    MemoryEnforcer(cl_device_id deviceId, cl_context context);
    ~MemoryEnforcer();

    bool allocatePartition(const std::string& partitionId, size_t memoryLimit);
    bool canAllocate(const std::string& partitionId, size_t requestedBytes) const;
    bool trackBuffer(const std::string& partitionId, cl_mem buffer, size_t size);
    bool releaseBuffer(const std::string& partitionId, cl_mem buffer);
    size_t getCurrentUsage(const std::string& partitionId) const;
    size_t getMemoryLimit(const std::string& partitionId) const;
    void releasePartition(const std::string& partitionId);
    std::vector<std::string> getActivePartitions() const;

   private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}  // namespace core
}  // namespace chronos

#endif
