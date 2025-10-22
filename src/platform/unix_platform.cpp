/**
 * @file unix_platform.cpp
 * @brief Unix-specific platform implementation
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

#include "platform/unix_platform.h"

#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>

namespace chronos {
namespace platform {

static UnixPlatform instance;

UnixPlatform::UnixPlatform() = default;
UnixPlatform::~UnixPlatform() = default;

bool UnixPlatform::createDirectory(const std::string& path, int permissions) {
    return mkdir(path.c_str(), static_cast<mode_t>(permissions)) == 0 || errno == EEXIST;
}

int UnixPlatform::getProcessId() { return getpid(); }

std::string UnixPlatform::getUsername() {
    struct passwd* pw = getpwuid(getuid());
    if (pw) {
        return std::string(pw->pw_name);
    }
    return "unknown";
}

std::string UnixPlatform::getHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "unknown-host";
}

std::string UnixPlatform::getTempPath() { return "/tmp/"; }

bool UnixPlatform::createLockFile(const std::string& path, const std::string& content) {
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        return false;
    }

    ssize_t written = write(fd, content.c_str(), content.size());
    if (written == -1 || static_cast<size_t>(written) != content.size()) {
        close(fd);
        unlink(path.c_str());
        return false;
    }

    fsync(fd);
    close(fd);
    return true;
}

bool UnixPlatform::deleteFile(const std::string& path) { return unlink(path.c_str()) == 0; }

bool UnixPlatform::fileExists(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return S_ISREG(buffer.st_mode);
    }
    return false;
}

std::string UnixPlatform::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string UnixPlatform::getCurrentTimeString() {
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_now);
    return std::string(timeStr);
}

Platform* Platform::getInstance() { return &instance; }

}  // namespace platform
}  // namespace chronos
