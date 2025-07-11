/**
 * @file chronos_utils.cpp
 * @brief Implementation of utility functions
 *
 * This file contains the implementation of the utility functions used
 * throughout the Chronos system, including functions for printing usage
 * information, parsing time strings, and formatting byte sizes and durations.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#include "chronos_utils.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace chronos {
namespace ChronosUtils {

void printUsage() {
  std::cout << "Chronos GPU Partitioner (OpenCL Version) - A time-based GPU "
               "partitioning utility"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout
      << "  chronos create <device_index> <memory_fraction> <duration_seconds>"
      << std::endl;
  std::cout << "  chronos list" << std::endl;
  std::cout << "  chronos release <partition_id>" << std::endl;
  std::cout << "  chronos stats" << std::endl;
  std::cout << "  chronos available <device_index>" << std::endl;
  std::cout << "  chronos help" << std::endl;
  std::cout << std::endl;
  std::cout << "Examples:" << std::endl;
  std::cout << "  chronos create 0 0.5 3600     # Use 50% of GPU 0 for 1 hour"
            << std::endl;
  std::cout << "  chronos list                  # List all active partitions"
            << std::endl;
  std::cout << "  chronos release partition_0001 # Release partition early"
            << std::endl;
  std::cout << "  chronos stats                 # Show device statistics"
            << std::endl;
  std::cout
      << "  chronos available 0           # Get percentage of GPU 0 available"
      << std::endl;
}

std::chrono::system_clock::time_point
parseTimeString(const std::string &timeStr) {
  std::tm tm = {};
  std::istringstream ss(timeStr);
  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

  if (ss.fail()) {
    throw std::runtime_error(
        "Failed to parse time string. Expected format: YYYY-MM-DDThh:mm:ss");
  }

  return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string formatTimePoint(const std::chrono::system_clock::time_point &time) {
  std::time_t t = std::chrono::system_clock::to_time_t(time);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

std::string formatByteSize(uint64_t bytes) {
  static const char *suffixes[] = {"B", "KB", "MB", "GB", "TB", "PB"};
  int suffixIndex = 0;
  double size = static_cast<double>(bytes);

  while (size >= 1024 && suffixIndex < 5) {
    size /= 1024;
    suffixIndex++;
  }

  std::stringstream ss;
  ss << std::fixed << std::setprecision(2) << size << " "
     << suffixes[suffixIndex];
  return ss.str();
}

std::string formatDuration(int seconds) {
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;

  std::stringstream ss;
  if (hours > 0) {
    ss << hours << "h ";
  }
  if (hours > 0 || minutes > 0) {
    ss << minutes << "m ";
  }
  ss << secs << "s";

  return ss.str();
}

} // namespace ChronosUtils
} // namespace chronos