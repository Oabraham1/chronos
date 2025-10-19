/**
 * @file lock_file.cpp
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

#include "utils/lock_file.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "platform/platform.h"

namespace chronos {
namespace utils {

class LockFile::Impl {
   public:
    Impl(const std::string& basePath) : basePath(basePath) {}

    std::string basePath;
};

LockFile::LockFile(const std::string& basePath) : pImpl(std::make_unique<Impl>(basePath)) {}

LockFile::~LockFile() = default;

bool LockFile::initializeLockDirectory() {
    return platform::Platform::getInstance()->createDirectory(pImpl->basePath);
}

std::string LockFile::generateLockFilePath(int deviceIdx, float memoryFraction) const {
    int memPercent = static_cast<int>(std::round(memoryFraction * 1000));

    std::stringstream ss;
    ss << pImpl->basePath << "gpu_" << deviceIdx << "_" << std::setw(4) << std::setfill('0')
       << memPercent << ".lock";

    return ss.str();
}

bool LockFile::createLock(int deviceIdx, float memoryFraction, const std::string& partitionId,
                          const std::string& username) {
    auto platform = platform::Platform::getInstance();
    std::string lockFilePath = generateLockFilePath(deviceIdx, memoryFraction);

    int pid = platform->getProcessId();
    std::string actualUsername = username.empty() ? platform->getUsername() : username;
    std::string hostname = platform->getHostname();
    std::string timestamp = platform->getCurrentTimeString();

    std::stringstream content;
    content << "pid: " << pid << "\n"
            << "user: " << actualUsername << "\n"
            << "host: " << hostname << "\n"
            << "time: " << timestamp << "\n"
            << "device: " << deviceIdx << "\n"
            << "fraction: " << memoryFraction << "\n"
            << "partition: " << partitionId << "\n";

    return platform->createLockFile(lockFilePath, content.str());
}

bool LockFile::releaseLock(int deviceIdx, float memoryFraction) {
    std::string lockFilePath = generateLockFilePath(deviceIdx, memoryFraction);
    return platform::Platform::getInstance()->deleteFile(lockFilePath);
}

bool LockFile::lockExists(int deviceIdx, float memoryFraction) const {
    std::string lockFilePath = generateLockFilePath(deviceIdx, memoryFraction);
    return platform::Platform::getInstance()->fileExists(lockFilePath);
}

std::string LockFile::getLockOwner(int deviceIdx, float memoryFraction) const {
    if (!lockExists(deviceIdx, memoryFraction)) {
        return "";
    }

    std::string lockFilePath = generateLockFilePath(deviceIdx, memoryFraction);
    std::string content = platform::Platform::getInstance()->readFile(lockFilePath);

    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.compare(0, 6, "user: ") == 0) {
            return line.substr(6);
        }
    }

    return "";
}

}  // namespace utils
}  // namespace chronos
