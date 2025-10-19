/**
 * @file commands.h
 * @brief Command handlers for CLI application
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
int executeCreate(ChronosPartitioner& partitioner, int argc, char* argv[]);

/**
 * @brief Execute the 'list' command
 *
 * @param partitioner Reference to partitioner object
 * @return Exit code
 */
int executeList(ChronosPartitioner& partitioner);

/**
 * @brief Execute the 'release' command
 *
 * @param partitioner Reference to partitioner object
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int executeRelease(ChronosPartitioner& partitioner, int argc, char* argv[]);

/**
 * @brief Execute the 'stats' command
 *
 * @param partitioner Reference to partitioner object
 * @return Exit code
 */
int executeStats(ChronosPartitioner& partitioner);

/**
 * @brief Execute the 'available' command
 *
 * @param partitioner Reference to partitioner object
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int executeAvailable(ChronosPartitioner& partitioner, int argc, char* argv[]);

/**
 * @brief Execute the 'help' command
 *
 * @return Exit code
 */
int executeHelp();

}  // namespace cli
}  // namespace chronos

#endif
