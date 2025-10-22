/**
 * @file test_user_permissions.cpp
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

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "chronos.h"
#include "platform/platform.h"

int testBasicUserAssignment() {
    try {
        chronos::ChronosPartitioner partitioner;
        auto platform = chronos::platform::Platform::getInstance();
        std::string currentUser = platform->getUsername();

        float memoryFraction = 0.1f;
        int duration = 5;

        std::string partitionId = partitioner.createPartition(0, memoryFraction, duration);
        if (partitionId.empty()) {
            std::cout << "No OpenCL devices - skipping test" << std::endl;
            return 0;
        }

        auto partitions = partitioner.listPartitions(false);
        assert(partitions.size() == 1);
        assert(partitions[0].username == currentUser);

        partitioner.releasePartition(partitionId);

        std::cout << "Basic user assignment test passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in testBasicUserAssignment: " << e.what() << std::endl;
        return 1;
    }
}

int testExplicitUserAssignment() {
    try {
        chronos::ChronosPartitioner partitioner;
        auto platform = chronos::platform::Platform::getInstance();
        std::string currentUser = platform->getUsername();

        float memoryFraction = 0.1f;
        int duration = 5;

        std::string partitionId =
            partitioner.createPartition(0, memoryFraction, duration, currentUser);
        if (partitionId.empty()) {
            std::cout << "No OpenCL devices - skipping test" << std::endl;
            return 0;
        }

        auto partitions = partitioner.listPartitions(false);
        assert(partitions.size() == 1);
        assert(partitions[0].username == currentUser);

        partitioner.releasePartition(partitionId);

        std::cout << "Explicit user assignment test passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in testExplicitUserAssignment: " << e.what() << std::endl;
        return 1;
    }
}

int testNonAdminCannotCreateForOthers() {
    try {
        chronos::ChronosPartitioner partitioner;
        auto platform = chronos::platform::Platform::getInstance();
        std::string currentUser = platform->getUsername();

        bool isAdmin = false;
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
        isAdmin = (currentUser == "root") || (geteuid() == 0);
#endif

        if (isAdmin) {
            std::cout << "Running as admin - skipping non-admin test" << std::endl;
            return 0;
        }

        float memoryFraction = 0.1f;
        int duration = 5;
        std::string otherUser = "definitely_not_" + currentUser;

        std::string partitionId =
            partitioner.createPartition(0, memoryFraction, duration, otherUser);

        assert(partitionId.empty());

        auto partitions = partitioner.listPartitions(false);
        assert(partitions.empty());

        std::cout << "Non-admin permission denial test passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in testNonAdminCannotCreateForOthers: " << e.what() << std::endl;
        return 1;
    }
}

int testOwnershipProtection() {
    try {
        chronos::ChronosPartitioner partitioner;
        auto platform = chronos::platform::Platform::getInstance();
        std::string currentUser = platform->getUsername();

        float memoryFraction = 0.1f;
        int duration = 10;

        std::string partitionId = partitioner.createPartition(0, memoryFraction, duration);
        if (partitionId.empty()) {
            std::cout << "No OpenCL devices - skipping test" << std::endl;
            return 0;
        }

        auto partitions = partitioner.listPartitions(false);
        assert(partitions.size() == 1);
        assert(partitions[0].username == currentUser);

        bool released = partitioner.releasePartition(partitionId);
        assert(released);

        partitions = partitioner.listPartitions(false);
        assert(partitions.empty());

        std::cout << "Ownership protection test passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in testOwnershipProtection: " << e.what() << std::endl;
        return 1;
    }
}

int testMultipleUsersSimulation() {
    try {
        chronos::ChronosPartitioner partitioner;
        auto platform = chronos::platform::Platform::getInstance();
        std::string currentUser = platform->getUsername();

        float memoryFraction = 0.05f;
        int duration = 10;

        std::vector<std::string> partitionIds;
        for (int i = 0; i < 3; i++) {
            std::string partitionId = partitioner.createPartition(0, memoryFraction, duration);
            if (partitionId.empty()) {
                if (i == 0) {
                    std::cout << "No OpenCL devices - skipping test" << std::endl;
                    return 0;
                }
                break;
            }
            partitionIds.push_back(partitionId);
        }

        auto partitions = partitioner.listPartitions(true);
        assert(partitions.size() == partitionIds.size());

        for (const auto& partition : partitions) {
            assert(partition.username == currentUser);
        }

        for (const auto& pid : partitionIds) {
            partitioner.releasePartition(pid);
        }

        partitions = partitioner.listPartitions(false);
        assert(partitions.empty());

        std::cout << "Multiple users simulation test passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in testMultipleUsersSimulation: " << e.what() << std::endl;
        return 1;
    }
}

int main() {
    int result = 0;

    std::cout << "==== Testing User Permissions ====" << std::endl;

    std::cout << "\n1. Basic User Assignment:" << std::endl;
    result += testBasicUserAssignment();

    std::cout << "\n2. Explicit User Assignment:" << std::endl;
    result += testExplicitUserAssignment();

    std::cout << "\n3. Non-Admin Permission Denial:" << std::endl;
    result += testNonAdminCannotCreateForOthers();

    std::cout << "\n4. Ownership Protection:" << std::endl;
    result += testOwnershipProtection();

    std::cout << "\n5. Multiple Users Simulation:" << std::endl;
    result += testMultipleUsersSimulation();

    std::cout << "\n==== Test Summary ====" << std::endl;
    if (result == 0) {
        std::cout << "All user permissions tests passed!" << std::endl;
    } else {
        std::cout << result << " tests failed" << std::endl;
    }

    return result;
}
