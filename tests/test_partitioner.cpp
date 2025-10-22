/**
 * @file test_partitioner.cpp
 * @brief Tests for the ChronosPartitioner class.
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

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#include "chronos.h"
#include "platform/opencl_include.h"

/**
 * @brief Test basic partition creation and listing
 * @return 0 if successful, non-zero otherwise
 */
int testPartitionBasic() {
    try {
        // Create partitioner instance
        chronos::ChronosPartitioner partitioner;

        // Check if we have any OpenCL devices
        auto partitions = partitioner.listPartitions(false);
        std::cout << "Initial partitions count: " << partitions.size() << std::endl;

        // Try to create a small partition on device 0 for a short time
        float memoryFraction = 0.1f;  // Just 10% of memory
        int durationSeconds = 5;      // 5 seconds

        std::string partitionId = partitioner.createPartition(0, memoryFraction, durationSeconds);
        if (partitionId.empty()) {
            std::cout << "Could not create partition (possibly no OpenCL devices "
                         "available) - skipping test in CI environment"
                      << std::endl;
            // This shouldn't fail the test as the device might not be available
            return 0;
        }

        // Verify partition was created
        partitions = partitioner.listPartitions(false);
        assert(partitions.size() == 1);
        assert(partitions[0].partitionId == partitionId);
        assert(partitions[0].active);
        assert(std::abs(partitions[0].memoryFraction - memoryFraction) < 0.0001f);
        assert(partitions[0].duration.count() == durationSeconds);

        // Check device stats (this also verifies the function doesn't crash)
        partitioner.showDeviceStats();

        // Check available percentage
        float available = partitioner.getGPUAvailablePercentage(0);
        std::cout << "Available percentage: " << available << "%" << std::endl;
        assert(available >= 0.0f && available <= 100.0f);

        // Release the partition
        bool released = partitioner.releasePartition(partitionId);
        assert(released);

        // Verify partition was released
        partitions = partitioner.listPartitions(false);
        assert(partitions.empty());

        std::cout << "Basic partition test passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in testPartitionBasic: " << e.what() << std::endl;
        return 1;
    }
}

/**
 * @brief Test automatic partition expiration
 * @return 0 if successful, non-zero otherwise
 */
int testPartitionExpiration() {
    try {
        // Create partitioner instance
        chronos::ChronosPartitioner partitioner;

        // Create a partition with a very short duration
        float memoryFraction = 0.1f;
        int durationSeconds = 2;  // Just 2 seconds

        std::string partitionId = partitioner.createPartition(0, memoryFraction, durationSeconds);
        if (partitionId.empty()) {
            std::cout << "Could not create partition (possibly no OpenCL devices "
                         "available) - skipping test in CI environment"
                      << std::endl;
            // This shouldn't fail the test as the device might not be available
            return 0;
        }

        // Verify partition was created
        auto partitions = partitioner.listPartitions(false);
        assert(partitions.size() == 1);

        // Wait for slightly longer than the duration to ensure it expires
        std::cout << "Waiting for partition to expire..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(durationSeconds + 1));

        // Verify partition was automatically released
        partitions = partitioner.listPartitions(false);
        assert(partitions.empty());

        std::cout << "Partition expiration test passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in testPartitionExpiration: " << e.what() << std::endl;
        return 1;
    }
}

/**
 * @brief Test multiple partitions on the same device
 * @return 0 if successful, non-zero otherwise
 */
int testMultiplePartitions() {
    try {
        // Create partitioner instance
        chronos::ChronosPartitioner partitioner;

        // Create multiple small partitions
        const int numPartitions = 3;
        float memoryFraction = 0.05f;  // 5% each
        int durationSeconds = 10;

        std::vector<std::string> partitionIds;

        for (int i = 0; i < numPartitions; i++) {
            std::string partitionId =
                partitioner.createPartition(0, memoryFraction, durationSeconds);
            if (partitionId.empty()) {
                // If we can't create the first partition, skip the test
                if (i == 0) {
                    std::cout << "Could not create partition (possibly no OpenCL devices "
                                 "available) - skipping test in CI environment"
                              << std::endl;
                    return 0;
                }

                // If we created some partitions but can't create more, that's expected
                std::cout << "Created " << i << " partitions before running out of memory"
                          << std::endl;
                break;
            }

            partitionIds.push_back(partitionId);
        }

        // Verify partitions were created
        auto partitions = partitioner.listPartitions(false);
        assert(partitions.size() == partitionIds.size());

        // Show partitions
        partitioner.listPartitions(true);

        // Release partitions in reverse order
        for (auto it = partitionIds.rbegin(); it != partitionIds.rend(); ++it) {
            bool released = partitioner.releasePartition(*it);
            assert(released);

            // Verify partition was released
            partitions = partitioner.listPartitions(false);
            assert(partitions.size() == std::distance(it + 1, partitionIds.rend()));
        }

        // Verify all partitions are released
        partitions = partitioner.listPartitions(false);
        assert(partitions.empty());

        std::cout << "Multiple partitions test passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in testMultiplePartitions: " << e.what() << std::endl;
        return 1;
    }
}

/**
 * @brief Test error handling for invalid inputs
 * @return 0 if successful, non-zero otherwise
 */
int testErrorHandling() {
    try {
        // Create partitioner instance
        chronos::ChronosPartitioner partitioner;

        // Test invalid device index
        std::string partitionId = partitioner.createPartition(-1, 0.5f, 10);
        assert(partitionId.empty());

        // Test invalid memory fraction
        partitionId = partitioner.createPartition(0, 0.0f, 10);
        assert(partitionId.empty());

        partitionId = partitioner.createPartition(0, 1.5f, 10);
        assert(partitionId.empty());

        // Test invalid duration
        partitionId = partitioner.createPartition(0, 0.5f, 0);
        assert(partitionId.empty());

        // Test releasing non-existent partition
        bool released = partitioner.releasePartition("non_existent_partition");
        assert(!released);

        // Test getting stats for invalid device
        float available = partitioner.getGPUAvailablePercentage(-1);
        assert(available < 0.0f);

        std::cout << "Error handling test passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in testErrorHandling: " << e.what() << std::endl;
        return 1;
    }
}

/**
 * @brief Main entry point
 * @return Exit code
 */
int main() {
    int result = 0;

    std::cout << "==== Testing ChronosPartitioner ====" << std::endl;

    std::cout << "\n1. Basic Partition Test:" << std::endl;
    result += testPartitionBasic();

    std::cout << "\n2. Partition Expiration Test:" << std::endl;
    result += testPartitionExpiration();

    std::cout << "\n3. Multiple Partitions Test:" << std::endl;
    result += testMultiplePartitions();

    std::cout << "\n4. Error Handling Test:" << std::endl;
    result += testErrorHandling();

    std::cout << "\n==== Test Summary ====" << std::endl;
    if (result == 0) {
        std::cout << "All tests passed successfully!" << std::endl;
    } else {
        std::cout << result << " tests failed." << std::endl;
    }

    return result;
}
