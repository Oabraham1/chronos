/**
 * @file windows_platform.h
 * @brief Windows-specific platform implementation header
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

#ifndef CHRONOS_WINDOWS_PLATFORM_H
#define CHRONOS_WINDOWS_PLATFORM_H

#include "platform/platform.h"

namespace chronos {
namespace platform {

/**
 * @class WindowsPlatform
 * @brief Windows-specific implementation of the Platform interface
 */
class WindowsPlatform : public Platform {
   public:
    /**
     * @brief Constructor
     */
    WindowsPlatform();

    /**
     * @brief Destructor
     */
    ~WindowsPlatform() override;

    /**
     * @brief Create a directory
     *
     * @param path Directory path
     * @param permissions Directory permissions (ignored on Windows)
     * @return True if successful, false otherwise
     */
    bool createDirectory(const std::string& path, int permissions = 0755) override;

    /**
     * @brief Get current process ID
     *
     * @return Process ID
     */
    int getProcessId() override;

    /**
     * @brief Get current username
     *
     * @return Username
     */
    std::string getUsername() override;

    /**
     * @brief Get hostname
     *
     * @return Hostname
     */
    std::string getHostname() override;

    /**
     * @brief Get temp directory path
     *
     * @return Path to temp directory with trailing separator
     */
    std::string getTempPath() override;

    /**
     * @brief Create a lock file atomically
     *
     * @param path Path to lock file
     * @param content Content to write to the file
     * @return True if successful, false otherwise
     */
    bool createLockFile(const std::string& path, const std::string& content) override;

    /**
     * @brief Delete a file
     *
     * @param path Path to file
     * @return True if successful, false otherwise
     */
    bool deleteFile(const std::string& path) override;

    /**
     * @brief Check if a file exists
     *
     * @param path Path to file
     * @return True if file exists, false otherwise
     */
    bool fileExists(const std::string& path) override;

    /**
     * @brief Read file content
     *
     * @param path Path to file
     * @return File content or empty string on error
     */
    std::string readFile(const std::string& path) override;

    /**
     * @brief Get current timestamp as formatted string
     *
     * @return Timestamp string in format "YYYY-MM-DD HH:MM:SS"
     */
    std::string getCurrentTimeString() override;
};

}  // namespace platform
}  // namespace chronos

#endif
