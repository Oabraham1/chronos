/**
 * @file lock_file.h
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

#ifndef CHRONOS_LOCK_FILE_H
#define CHRONOS_LOCK_FILE_H

#include <memory>
#include <string>

namespace chronos {
namespace utils {

class LockFile {
   public:
    explicit LockFile(const std::string& basePath);
    ~LockFile();

    bool initializeLockDirectory();
    std::string generateLockFilePath(int deviceIdx, float memoryFraction) const;
    bool createLock(int deviceIdx, float memoryFraction, const std::string& partitionId,
                    const std::string& username = "");
    bool releaseLock(int deviceIdx, float memoryFraction);
    bool lockExists(int deviceIdx, float memoryFraction) const;
    std::string getLockOwner(int deviceIdx, float memoryFraction) const;

   private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}  // namespace utils
}  // namespace chronos

#endif
