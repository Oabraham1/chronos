/**
 * @file platform.h
 * @brief Platform abstraction interface
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

#ifndef CHRONOS_PLATFORM_H
#define CHRONOS_PLATFORM_H

#include <string>

namespace chronos {
namespace platform {

/**
 * @class Platform
 * @brief Abstract base class for platform-specific operations
 *
 * This class provides an interface for platform-specific operations,
 * allowing the core code to be platform-independent.
 */
class Platform {
   public:
    /**
     * @brief Destructor
     */
    virtual ~Platform() = default;

    /**
     * @brief Create a directory
     *
     * @param path Directory path
     * @param permissions Directory permissions (ignored on Windows)
     * @return True if successful, false otherwise
     */
    virtual bool createDirectory(const std::string& path, int permissions = 0755) = 0;

    /**
     * @brief Get current process ID
     *
     * @return Process ID
     */
    virtual int getProcessId() = 0;

    /**
     * @brief Get current username
     *
     * @return Username
     */
    virtual std::string getUsername() = 0;

    /**
     * @brief Get hostname
     *
     * @return Hostname
     */
    virtual std::string getHostname() = 0;

    /**
     * @brief Get temp directory path
     *
     * @return Path to temp directory with trailing separator
     */
    virtual std::string getTempPath() = 0;

    /**
     * @brief Create a lock file atomically
     *
     * @param path Path to lock file
     * @param content Content to write to the file
     * @return True if successful, false otherwise
     */
    virtual bool createLockFile(const std::string& path, const std::string& content) = 0;

    /**
     * @brief Delete a file
     *
     * @param path Path to file
     * @return True if successful, false otherwise
     */
    virtual bool deleteFile(const std::string& path) = 0;

    /**
     * @brief Check if a file exists
     *
     * @param path Path to file
     * @return True if file exists, false otherwise
     */
    virtual bool fileExists(const std::string& path) = 0;

    /**
     * @brief Read file content
     *
     * @param path Path to file
     * @return File content or empty string on error
     */
    virtual std::string readFile(const std::string& path) = 0;

    /**
     * @brief Get current timestamp as formatted string
     *
     * @return Timestamp string in format "YYYY-MM-DD HH:MM:SS"
     */
    virtual std::string getCurrentTimeString() = 0;

    /**
     * @brief Get platform-specific instance
     *
     * @return Platform instance for current operating system
     */
    static Platform* getInstance();
};

}  // namespace platform
}  // namespace chronos

#endif
