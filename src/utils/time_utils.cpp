/**
 * @file time_utils.cpp
 * @brief Implementation of time utility functions
 */

#include "utils/time_utils.h"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace chronos
{
    namespace utils
    {

        std::chrono::system_clock::time_point TimeUtils::getCurrentTime()
        {
            return std::chrono::system_clock::now();
        }

        std::string TimeUtils::formatIso8601(const std::chrono::system_clock::time_point &time)
        {
            std::time_t t = std::chrono::system_clock::to_time_t(time);
            std::tm *tm = std::localtime(&t);

            std::stringstream ss;
            ss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
            return ss.str();
        }

        std::string TimeUtils::formatHumanReadable(const std::chrono::system_clock::time_point &time)
        {
            std::time_t t = std::chrono::system_clock::to_time_t(time);
            std::tm *tm = std::localtime(&t);

            std::stringstream ss;
            ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
            return ss.str();
        }

        std::chrono::system_clock::time_point TimeUtils::parseIso8601(const std::string &isoString)
        {
            std::tm tm = {};
            std::stringstream ss(isoString);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

            if (ss.fail())
            {
                throw std::runtime_error("Failed to parse ISO 8601 time string. Expected format: YYYY-MM-DDThh:mm:ss");
            }

            return std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }

        std::string TimeUtils::formatDuration(const std::chrono::seconds &duration)
        {
            auto secs = duration.count();

            int hours = static_cast<int>(secs / 3600);
            int minutes = static_cast<int>((secs % 3600) / 60);
            int seconds = static_cast<int>(secs % 60);

            std::stringstream ss;
            if (hours > 0)
            {
                ss << hours << "h ";
            }
            if (hours > 0 || minutes > 0)
            {
                ss << minutes << "m ";
            }
            ss << seconds << "s";

            return ss.str();
        }

    }
}
