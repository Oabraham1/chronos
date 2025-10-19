/**
 * @file test_lock_file.cpp
 * @brief Tests for the LockFile class.
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

#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "platform/platform.h"
#include "utils/lock_file.h"

/**
 * @brief Test basic lock file functionality
 * @return 0 if successful, non-zero otherwise
 */
int testLockFileBasic() {
    auto platform = chronos::platform::Platform::getInstance();
    std::stringstream lockDir;
    lockDir << platform->getTempPath() << "chronos_test_locks_" << platform->getProcessId() << "/";

    chronos::utils::LockFile lockFile(lockDir.str());
    assert(lockFile.initializeLockDirectory());

    int deviceIdx = 0;
    float memoryFraction = 0.5f;
    std::string partitionId = "test_partition_0001";

    assert(lockFile.createLock(deviceIdx, memoryFraction, partitionId));

    assert(lockFile.lockExists(deviceIdx, memoryFraction));

    std::string owner = lockFile.getLockOwner(deviceIdx, memoryFraction);
    assert(owner == platform->getUsername());

    assert(lockFile.releaseLock(deviceIdx, memoryFraction));

    assert(!lockFile.lockExists(deviceIdx, memoryFraction));

    platform->deleteFile(lockDir.str());

    std::cout << "LockFile basic test passed" << std::endl;
    return 0;
}

/**
 * @brief Test multiple lock files
 * @return 0 if successful, non-zero otherwise
 */
int testLockFileMultiple() {
    auto platform = chronos::platform::Platform::getInstance();
    std::stringstream lockDir;
    lockDir << platform->getTempPath() << "chronos_test_locks_" << platform->getProcessId() << "/";

    chronos::utils::LockFile lockFile(lockDir.str());
    assert(lockFile.initializeLockDirectory());

    const int numLocks = 5;
    std::vector<std::pair<int, float>> locks;

    for (int i = 0; i < numLocks; i++) {
        int deviceIdx = i % 2;
        float memoryFraction = 0.1f * (i + 1);
        std::string partitionId = "test_partition_" + std::to_string(1000 + i);

        assert(lockFile.createLock(deviceIdx, memoryFraction, partitionId));
        locks.push_back({deviceIdx, memoryFraction});
    }

    for (const auto& lock : locks) {
        assert(lockFile.lockExists(lock.first, lock.second));
        assert(lockFile.getLockOwner(lock.first, lock.second) == platform->getUsername());
    }

    std::reverse(locks.begin(), locks.end());
    for (const auto& lock : locks) {
        assert(lockFile.releaseLock(lock.first, lock.second));
        assert(!lockFile.lockExists(lock.first, lock.second));
    }

    platform->deleteFile(lockDir.str());

    std::cout << "LockFile multiple test passed" << std::endl;
    return 0;
}

/**
 * @brief Main entry point
 * @return Exit code
 */
int main() {
    int result = 0;

    result += testLockFileBasic();
    result += testLockFileMultiple();

    return result;
}
