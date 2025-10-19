/**
 * @file simple_partition.cpp
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

#include <chrono>
#include <iostream>
#include <thread>

#include "chronos.h"

void simulateGpuWorkload(const std::string& partitionId, int durationSeconds) {
    std::cout << "Starting GPU workload on partition " << partitionId << std::endl;
    std::cout << "Working";

    for (int i = 0; i < durationSeconds; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "." << std::flush;
    }

    std::cout << std::endl << "GPU workload completed" << std::endl;
}

int main(int argc, char* argv[]) {
    chronos::ChronosPartitioner partitioner;

    partitioner.showDeviceStats();

    int deviceIndex = 0;
    float memoryFraction = 0.5;
    int duration = 30;

    std::cout << "Creating GPU partition..." << std::endl;
    std::string partitionId = partitioner.createPartition(deviceIndex, memoryFraction, duration);

    if (partitionId.empty()) {
        std::cerr << "Failed to create partition" << std::endl;
        return 1;
    }

    partitioner.listPartitions(true);

    simulateGpuWorkload(partitionId, 10);

    std::cout << "Releasing partition..." << std::endl;
    partitioner.releasePartition(partitionId);

    partitioner.showDeviceStats();

    return 0;
}
