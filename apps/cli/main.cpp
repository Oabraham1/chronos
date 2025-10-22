/**
 * @file main.cpp
 * @brief Command-line interface for Chronos GPU Partitioner.
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

#include <iostream>
#include <stdexcept>
#include <string>

#include "chronos.h"
#include "chronos_utils.h"
#include "commands.h"

/**
 * @brief Main entry point
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        chronos::ChronosUtils::printUsage();
        return 1;
    }

    std::string command = argv[1];

    try {
        chronos::ChronosPartitioner partitioner;

        if (command == "create") {
            return chronos::cli::executeCreate(partitioner, argc, argv);
        } else if (command == "list") {
            return chronos::cli::executeList(partitioner);
        } else if (command == "release") {
            return chronos::cli::executeRelease(partitioner, argc, argv);
        } else if (command == "stats") {
            return chronos::cli::executeStats(partitioner);
        } else if (command == "available") {
            return chronos::cli::executeAvailable(partitioner, argc, argv);
        } else if (command == "help") {
            return chronos::cli::executeHelp();
        } else {
            std::cerr << "Invalid command: " << command << std::endl;
            chronos::ChronosUtils::printUsage();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
