/**
 * @file time_utils.h
 * @brief Time-related utility functions
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

#ifndef CHRONOS_TIME_UTILS_H
#define CHRONOS_TIME_UTILS_H

#include <chrono>
#include <string>

namespace chronos {
namespace utils {

/**
 * @class TimeUtils
 * @brief Time-related utilities
 */
class TimeUtils {
   public:
    /**
     * @brief Get current system time
     *
     * @return Current time point
     */
    static std::chrono::system_clock::time_point getCurrentTime();

    /**
     * @brief Format time point as ISO 8601 string
     *
     * @param time Time point to format
     * @return ISO 8601 formatted string (YYYY-MM-DDThh:mm:ss)
     */
    static std::string formatIso8601(const std::chrono::system_clock::time_point& time);

    /**
     * @brief Format time point as human-readable string
     *
     * @param time Time point to format
     * @return Human-readable string (YYYY-MM-DD hh:mm:ss)
     */
    static std::string formatHumanReadable(const std::chrono::system_clock::time_point& time);

    /**
     * @brief Parse ISO 8601 string to time point
     *
     * @param isoString ISO 8601 string (YYYY-MM-DDThh:mm:ss)
     * @return Parsed time point
     * @throws std::runtime_error if parsing fails
     */
    static std::chrono::system_clock::time_point parseIso8601(const std::string& isoString);

    /**
     * @brief Format duration as human-readable string
     *
     * @param duration Duration to format
     * @return Human-readable duration (e.g., "1h 23m 45s")
     */
    static std::string formatDuration(const std::chrono::seconds& duration);
};

}  // namespace utils
}  // namespace chronos

#endif
