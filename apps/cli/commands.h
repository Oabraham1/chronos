/**
 * @file commands.h
 * @brief Command handlers for CLI application
 *
 * This file contains the declarations for the command handler functions
 * used by the Chronos command-line interface (CLI). Each function
 * corresponds to a specific CLI command.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#ifndef CHRONOS_COMMANDS_H
#define CHRONOS_COMMANDS_H

#include "chronos.h"

namespace chronos {
namespace cli {

/**
 * @brief Execute the 'create' command
 *
 * @param partitioner Reference to partitioner object
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int executeCreate(ChronosPartitioner &partitioner, int argc, char *argv[]);

/**
 * @brief Execute the 'list' command
 *
 * @param partitioner Reference to partitioner object
 * @return Exit code
 */
int executeList(ChronosPartitioner &partitioner);

/**
 * @brief Execute the 'release' command
 *
 * @param partitioner Reference to partitioner object
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int executeRelease(ChronosPartitioner &partitioner, int argc, char *argv[]);

/**
 * @brief Execute the 'stats' command
 *
 * @param partitioner Reference to partitioner object
 * @return Exit code
 */
int executeStats(ChronosPartitioner &partitioner);

/**
 * @brief Execute the 'available' command
 *
 * @param partitioner Reference to partitioner object
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int executeAvailable(ChronosPartitioner &partitioner, int argc, char *argv[]);

/**
 * @brief Execute the 'help' command
 *
 * @return Exit code
 */
int executeHelp();

} // namespace cli
} // namespace chronos

#endif