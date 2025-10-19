/**
 * @file time_utils.cpp
 * @brief Implementation of time utility functions.
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

#include "utils/time_utils.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace chronos {
namespace utils {

std::chrono::system_clock::time_point TimeUtils::getCurrentTime() {
    return std::chrono::system_clock::now();
}

std::string TimeUtils::formatIso8601(const std::chrono::system_clock::time_point& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);

    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

std::string TimeUtils::formatHumanReadable(const std::chrono::system_clock::time_point& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);

    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::chrono::system_clock::time_point TimeUtils::parseIso8601(const std::string& isoString) {
    std::tm tm = {};
    std::stringstream ss(isoString);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) {
        throw std::runtime_error(
            "Failed to parse ISO 8601 time string. Expected "
            "format: YYYY-MM-DDThh:mm:ss");
    }

    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string TimeUtils::formatDuration(const std::chrono::seconds& duration) {
    auto secs = duration.count();

    int hours = static_cast<int>(secs / 3600);
    int minutes = static_cast<int>((secs % 3600) / 60);
    int seconds = static_cast<int>(secs % 60);

    std::stringstream ss;
    if (hours > 0) {
        ss << hours << "h ";
    }
    if (hours > 0 || minutes > 0) {
        ss << minutes << "m ";
    }
    ss << seconds << "s";

    return ss.str();
}

}  // namespace utils
}  // namespace chronos
