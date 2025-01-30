/**
 * @file simple_partition.cpp
 * @brief Simple example of using Chronos GPU Partitioner.
 *
 * This file demonstrates the basic usage of the Chronos library, including
 * creating a partition, running a simulated GPU workload, and releasing
 * the partition.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#include <chrono>
#include <iostream>
#include <thread>

#include "chronos.h"

/**
 * @brief Example function to simulate a GPU workload
 *
 * @param partitionId ID of the GPU partition
 * @param durationSeconds How long to run the simulation
 */
void simulateGpuWorkload(const std::string& partitionId, int durationSeconds) {
    std::cout << "Starting GPU workload on partition " << partitionId << std::endl;
    std::cout << "Working";

    for (int i = 0; i < durationSeconds; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "." << std::flush;
    }

    std::cout << std::endl << "GPU workload completed" << std::endl;
}

/**
 * @brief Main entry point
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
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