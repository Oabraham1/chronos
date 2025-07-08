/**
 * @file lock_file.h
 * @brief Lock file management
 *
 * This file defines the LockFile class, which manages the creation, reading,
 * and releasing of lock files used to coordinate GPU access between processes.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#ifndef CHRONOS_LOCK_FILE_H
#define CHRONOS_LOCK_FILE_H

#include <memory>
#include <string>

namespace chronos {
namespace utils {

/**
 * @class LockFile
 * @brief Manages lock files for GPU partitions
 *
 * This class handles creation, reading, and releasing of
 * lock files used to coordinate GPU access between processes.
 */
class LockFile {
public:
  /**
   * @brief Constructor
   *
   * @param basePath Base directory for lock files
   */
  explicit LockFile(const std::string &basePath);

  /**
   * @brief Destructor
   */
  ~LockFile();

  /**
   * @brief Initialize lock file directory
   *
   * @return True if successfully initialized, false otherwise
   */
  bool initializeLockDirectory();

  /**
   * @brief Generate lock file path for a device/fraction
   *
   * @param deviceIdx Device index
   * @param memoryFraction Memory fraction (0.0-1.0)
   * @return Path to lock file
   */
  std::string generateLockFilePath(int deviceIdx, float memoryFraction) const;

  /**
   * @brief Create a lock file
   *
   * @param deviceIdx Device index
   * @param memoryFraction Memory fraction
   * @param partitionId Partition ID
   * @return True if successfully created, false otherwise
   */
  bool createLock(int deviceIdx, float memoryFraction,
                  const std::string &partitionId);

  /**
   * @brief Release a lock file
   *
   * @param deviceIdx Device index
   * @param memoryFraction Memory fraction
   * @return True if successfully released, false otherwise
   */
  bool releaseLock(int deviceIdx, float memoryFraction);

  /**
   * @brief Check if a lock exists
   *
   * @param deviceIdx Device index
   * @param memoryFraction Memory fraction
   * @return True if lock exists, false otherwise
   */
  bool lockExists(int deviceIdx, float memoryFraction) const;

  /**
   * @brief Get lock file owner
   *
   * @param deviceIdx Device index
   * @param memoryFraction Memory fraction
   * @return Username of lock owner or empty string if not found
   */
  std::string getLockOwner(int deviceIdx, float memoryFraction) const;

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace utils
} // namespace chronos

#endif