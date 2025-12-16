/**
 * @file stub_backend.h
 * @brief Stub execution backend for testing and no-GPU environments
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

#ifndef CHRONOS_STUB_BACKEND_H
#define CHRONOS_STUB_BACKEND_H

#include "backends/execution_backend.h"

namespace chronos {
namespace backends {

/**
 * @class StubBackend
 * @brief No-op backend for testing and environments without GPU
 *
 * This backend provides the full ExecutionBackend interface but
 * doesn't actually interact with any GPU hardware. Useful for:
 * - Unit testing
 * - CI/CD pipelines
 * - Development on machines without GPUs
 */
class StubBackend : public ExecutionBackend {
   public:
    StubBackend();
    ~StubBackend() override = default;

    std::string getName() const override { return "Stub"; }
    ExecutionMode getExecutionMode() const override { return ExecutionMode::STUB; }
    bool isAvailable() const override { return true; }  // Always available
    std::string getDescription() const override {
        return "Stub backend (no GPU, for testing only)";
    }

    bool initialize() override;
    void shutdown() override;

    int getDeviceCount() const override;
    std::vector<BackendDeviceInfo> getDevices() const override;

    std::string createPartition(int deviceIndex, float memoryFraction, int durationSeconds,
                                const std::string& username) override;
    bool releasePartition(const std::string& partitionId) override;
    std::vector<BackendPartition> listPartitions() const override;

    float getAvailablePercentage(int deviceIndex) const override;

   private:
    bool initialized_;
    std::vector<BackendDeviceInfo> fakeDevices_;
    std::vector<BackendPartition> partitions_;
    int partitionCounter_;
};

}  // namespace backends
}  // namespace chronos

#endif  // CHRONOS_STUB_BACKEND_H
