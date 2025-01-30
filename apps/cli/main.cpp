/**
 * @file main.cpp
 * @brief Command-line interface for Chronos GPU Partitioner.
 *
 * This file serves as the entry point for the Chronos command-line
 * interface (CLI). It parses the command-line arguments and dispatches
 * them to the appropriate command handlers.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
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