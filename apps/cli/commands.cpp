/**
 * @file commands.cpp
 * @brief Description needed
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

#include "commands.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "chronos_utils.h"
#include "cli/formatter.h"

namespace chronos {
namespace cli {

int executeCreate(ChronosPartitioner& partitioner, int argc, char* argv[]) {
    if (argc < 5) {
        printError("'create' command requires device index, memory fraction, and duration");
        ChronosUtils::printUsage();
        return 1;
    }

    try {
        int deviceIdx = std::stoi(argv[2]);
        float memoryFraction = std::stof(argv[3]);
        int duration = std::stoi(argv[4]);

        if (memoryFraction <= 0.0f || memoryFraction > 1.0f) {
            printError("Memory fraction must be between 0 and 1");
            return 1;
        }

        if (duration <= 0) {
            printError("Duration must be positive");
            return 1;
        }

        std::string targetUser;
        for (int i = 5; i < argc; i++) {
            std::string arg(argv[i]);
            if (arg == "--user" && i + 1 < argc) {
                targetUser = argv[i + 1];
                i++;
            }
        }

        std::string partitionId =
            partitioner.createPartition(deviceIdx, memoryFraction, duration, targetUser);

        if (partitionId.empty()) {
            return 1;
        }

        printSuccess("Created partition " + partitionId);
        printInfo("Device: GPU " + std::to_string(deviceIdx));
        printInfo("Memory: " + std::to_string(static_cast<int>(memoryFraction * 100)) + "%");
        printInfo("Duration: " + formatDuration(duration));
        if (!targetUser.empty()) {
            printInfo("User: " + targetUser);
        }

        return 0;
    } catch (const std::invalid_argument&) {
        printError("Arguments must be numeric values");
        return 1;
    } catch (const std::out_of_range&) {
        printError("Argument value out of range");
        return 1;
    } catch (const std::exception& e) {
        printError(std::string("Failed to create partition: ") + e.what());
        return 1;
    }
}

int executeList(ChronosPartitioner& partitioner) {
    try {
        auto partitions = partitioner.listPartitions(false);

        if (partitions.empty()) {
            printInfo("No active partitions");
            return 0;
        }

        std::cout << "\n";
        if (shouldUseColors()) {
            std::cout << Color::BOLD << "Active Partitions" << Color::RESET << "\n\n";
        } else {
            std::cout << "Active Partitions\n\n";
        }

        Table table({"ID", "Device", "Memory", "Time Left", "Owner", "Status"});

        for (const auto& p : partitions) {
            std::vector<std::string> row;

            row.push_back(p.partitionId);

            row.push_back("GPU 0");

            std::ostringstream memStream;
            memStream << std::fixed << std::setprecision(0) << (p.memoryFraction * 100) << "%";
            row.push_back(memStream.str());

            row.push_back(formatDuration(p.getRemainingTime()));

            row.push_back(p.username);

            row.push_back(statusBadge(p.active ? "ACTIVE" : "EXPIRED"));

            table.addRow(row);
        }

        table.print();
        std::cout << "\n";
        printInfo("Total: " + std::to_string(partitions.size()) + " partition(s)");
        std::cout << "\n";

        return 0;
    } catch (const std::exception& e) {
        printError(std::string("Failed to list partitions: ") + e.what());
        return 1;
    }
}

int executeRelease(ChronosPartitioner& partitioner, int argc, char* argv[]) {
    if (argc != 3) {
        printError("'release' command requires partition ID");
        ChronosUtils::printUsage();
        return 1;
    }

    try {
        std::string partitionId = argv[2];

        if (!partitioner.releasePartition(partitionId)) {
            return 1;
        }

        printSuccess("Released partition " + partitionId);
        return 0;
    } catch (const std::exception& e) {
        printError(std::string("Failed to release partition: ") + e.what());
        return 1;
    }
}

int executeStats(ChronosPartitioner& partitioner) {
    try {
        std::cout << "\n";
        if (shouldUseColors()) {
            std::cout << Color::BOLD << "GPU Device Statistics" << Color::RESET << "\n";
        } else {
            std::cout << "GPU Device Statistics\n";
        }
        std::cout << std::string(50, '=') << "\n\n";

        partitioner.showDeviceStats();

        return 0;
    } catch (const std::exception& e) {
        printError(std::string("Failed to get device stats: ") + e.what());
        return 1;
    }
}

int executeAvailable(ChronosPartitioner& partitioner, int argc, char* argv[]) {
    if (argc != 3) {
        printError("'available' command requires device index");
        ChronosUtils::printUsage();
        return 1;
    }

    try {
        int deviceIdx = std::stoi(argv[2]);
        float availablePercent = partitioner.getGPUAvailablePercentage(deviceIdx);

        if (availablePercent >= 0) {
            if (shouldUseColors()) {
                std::string color;
                if (availablePercent >= 70) {
                    color = Color::GREEN;
                } else if (availablePercent >= 30) {
                    color = Color::YELLOW;
                } else {
                    color = Color::RED;
                }

                std::cout << color << std::fixed << std::setprecision(1) << availablePercent << "%"
                          << Color::RESET << " available on GPU " << deviceIdx << std::endl;
            } else {
                std::cout << std::fixed << std::setprecision(1) << availablePercent << "%"
                          << std::endl;
            }
            return 0;
        } else {
            return 1;
        }
    } catch (const std::invalid_argument&) {
        printError("Device index must be a numeric value");
        return 1;
    } catch (const std::out_of_range&) {
        printError("Device index out of range");
        return 1;
    } catch (const std::exception& e) {
        printError(std::string("Failed to get available percentage: ") + e.what());
        return 1;
    }
}

int executeHelp() {
    ChronosUtils::printUsage();
    return 0;
}

}  // namespace cli
}  // namespace chronos
