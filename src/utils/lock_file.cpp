/**
 * @file lock_file.cpp
 * @brief Implementation of the distributed lock management system for Chronos.
 *
 * This file provides a robust and thread-safe implementation of a distributed
 * lock manager. The locking mechanism is based on atomic file creation and
 * ensures that only one process can have exclusive access to a GPU partition
 * at any given time.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
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

bool LockFile::createLock(int deviceIdx, float memoryFraction, const std::string& partitionId) {
    auto platform = platform::Platform::getInstance();
    std::string lockFilePath = generateLockFilePath(deviceIdx, memoryFraction);

    int pid = platform->getProcessId();
    std::string username = platform->getUsername();
    std::string hostname = platform->getHostname();
    std::string timestamp = platform->getCurrentTimeString();

    std::stringstream content;
    content << "pid: " << pid << "\n"
            << "user: " << username << "\n"
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