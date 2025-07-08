/**
 * @file commands.cpp
 * @brief Implementation of command handlers for CLI application.
 *
 * This file contains the implementation of the command handler functions
 * for the Chronos command-line interface (CLI). Each function is
 * responsible for executing a specific CLI command, such as creating,
 * listing, or releasing a partition.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#include "commands.h"
#include "chronos_utils.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace chronos {
namespace cli {

int executeCreate(ChronosPartitioner &partitioner, int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "Error: 'create' command requires device index, memory "
                 "fraction, and duration"
              << std::endl;
    ChronosUtils::printUsage();
    return 1;
  }

  try {
    int deviceIdx = std::stoi(argv[2]);
    float memoryFraction = std::stof(argv[3]);
    int duration = std::stoi(argv[4]);

    if (memoryFraction <= 0.0f || memoryFraction > 1.0f) {
      std::cerr << "Error: memory fraction must be between 0 and 1"
                << std::endl;
      return 1;
    }

    if (duration <= 0) {
      std::cerr << "Error: duration must be positive" << std::endl;
      return 1;
    }

    std::string partitionId =
        partitioner.createPartition(deviceIdx, memoryFraction, duration);
    if (partitionId.empty()) {
      return 1;
    }
    return 0;
  } catch (const std::invalid_argument &) {
    std::cerr << "Error: arguments must be numeric values" << std::endl;
    return 1;
  } catch (const std::out_of_range &) {
    std::cerr << "Error: argument value out of range" << std::endl;
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Error creating partition: " << e.what() << std::endl;
    return 1;
  }
}

int executeList(ChronosPartitioner &partitioner) {
  try {
    partitioner.listPartitions(true);
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error listing partitions: " << e.what() << std::endl;
    return 1;
  }
}

int executeRelease(ChronosPartitioner &partitioner, int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Error: 'release' command requires partition ID" << std::endl;
    ChronosUtils::printUsage();
    return 1;
  }

  try {
    std::string partitionId = argv[2];
    if (!partitioner.releasePartition(partitionId)) {
      return 1;
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error releasing partition: " << e.what() << std::endl;
    return 1;
  }
}

int executeStats(ChronosPartitioner &partitioner) {
  try {
    partitioner.showDeviceStats();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error getting device stats: " << e.what() << std::endl;
    return 1;
  }
}

int executeAvailable(ChronosPartitioner &partitioner, int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Error: 'available' command requires device index"
              << std::endl;
    ChronosUtils::printUsage();
    return 1;
  }

  try {
    int deviceIdx = std::stoi(argv[2]);
    float availablePercent = partitioner.getGPUAvailablePercentage(deviceIdx);

    if (availablePercent >= 0) {
      std::cout << std::fixed << std::setprecision(2) << availablePercent
                << std::endl;
      return 0;
    } else {
      return 1;
    }
  } catch (const std::invalid_argument &) {
    std::cerr << "Error: device index must be a numeric value" << std::endl;
    return 1;
  } catch (const std::out_of_range &) {
    std::cerr << "Error: device index out of range" << std::endl;
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Error getting available percentage: " << e.what()
              << std::endl;
    return 1;
  }
}

int executeHelp() {
  ChronosUtils::printUsage();
  return 0;
}

} // namespace cli
} // namespace chronos