/**
 * @file windows_platform.cpp
 * @brief Windows-specific platform implementation.
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

#include "platform/windows_platform.h"

#include <direct.h>
#include <sddl.h>
#include <windows.h>

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>

namespace chronos {
namespace platform {

static WindowsPlatform instance;

WindowsPlatform::WindowsPlatform() = default;
WindowsPlatform::~WindowsPlatform() = default;

bool WindowsPlatform::createDirectory(const std::string& path, int permissions) {
    (void)permissions;
    return CreateDirectoryA(path.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

int WindowsPlatform::getProcessId() { return GetCurrentProcessId(); }

std::string WindowsPlatform::getUsername() {
    char username[256];
    DWORD size = sizeof(username);
    if (GetUserNameA(username, &size)) {
        return std::string(username);
    }
    return "unknown";
}

std::string WindowsPlatform::getHostname() {
    char hostname[256];
    DWORD hostnameSize = sizeof(hostname);
    if (GetComputerNameA(hostname, &hostnameSize)) {
        return std::string(hostname);
    }
    return "unknown-host";
}

std::string WindowsPlatform::getTempPath() {
    char tempPath[MAX_PATH + 1];
    DWORD length = GetTempPathA(MAX_PATH, tempPath);
    if (length > 0 && length <= MAX_PATH) {
        return std::string(tempPath);
    }
    return "C:\\Temp\\";
}

bool WindowsPlatform::createLockFile(const std::string& path, const std::string& content) {
    HANDLE hFile =
        CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesWritten;
    bool success = WriteFile(hFile, content.c_str(), static_cast<DWORD>(content.size()),
                             &bytesWritten, NULL) != 0;
    CloseHandle(hFile);

    if (!success || bytesWritten != content.size()) {
        DeleteFileA(path.c_str());
        return false;
    }

    return true;
}

bool WindowsPlatform::deleteFile(const std::string& path) { return DeleteFileA(path.c_str()) != 0; }

bool WindowsPlatform::fileExists(const std::string& path) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
        return !(fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }
    return false;
}

std::string WindowsPlatform::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string WindowsPlatform::getCurrentTimeString() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timeStr[32];
    sprintf_s(timeStr, sizeof(timeStr), "%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth,
              st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::string(timeStr);
}

Platform* Platform::getInstance() { return &instance; }

}  // namespace platform
}  // namespace chronos
