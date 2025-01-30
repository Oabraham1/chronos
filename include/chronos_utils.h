/**
 * @file chronos_utils.h
 * @brief Utility functions for Chronos GPU Partitioner
 *
 * This file provides a collection of utility functions used throughout the
 * Chronos system, including functions for printing usage information, parsing
 * time strings, and formatting byte sizes and durations.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#ifndef CHRONOS_UTILS_H
#define CHRONOS_UTILS_H

#include <chrono>
#include <string>

namespace chronos {

/**
 * @namespace ChronosUtils
 * @brief Utility functions for Chronos
 */
namespace ChronosUtils {

/**
 * @brief Print usage information
 */
void printUsage();

/**
 * @brief Parse a time string to system clock time point
 *
 * @param timeStr Time string in format "YYYY-MM-DDThh:mm:ss"
 * @return Time point
 * @throws std::runtime_error on parse failure
 */
std::chrono::system_clock::time_point parseTimeString(const std::string& timeStr);

/**
 * @brief Format a time point as string
 *
 * @param time Time point to format
 * @return Formatted time string "YYYY-MM-DD HH:MM:SS"
 */
std::string formatTimePoint(const std::chrono::system_clock::time_point& time);

/**
 * @brief Convert bytes to human-readable size
 *
 * @param bytes Size in bytes
 * @return Human-readable size (e.g., "1.23 GB")
 */
std::string formatByteSize(uint64_t bytes);

/**
 * @brief Format a duration as string
 *
 * @param seconds Duration in seconds
 * @return Formatted duration (e.g., "1h 23m 45s")
 */
std::string formatDuration(int seconds);

}  // namespace ChronosUtils

}  // namespace chronos

#endif