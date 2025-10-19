/**
 * @file formatter.h
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

#ifndef CHRONOS_CLI_FORMATTER_H
#define CHRONOS_CLI_FORMATTER_H

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// Platform-specific includes for isatty
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

namespace chronos {
namespace cli {

// ANSI color codes
namespace Color {
const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string DIM = "\033[2m";

const std::string BLACK = "\033[30m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string WHITE = "\033[37m";

const std::string BG_RED = "\033[41m";
const std::string BG_GREEN = "\033[42m";
const std::string BG_YELLOW = "\033[43m";
const std::string BG_BLUE = "\033[44m";
}  // namespace Color

inline bool shouldUseColors() {
#ifdef _WIN32
    // Windows 10+ supports ANSI colors
    return true;
#else
    // Unix: Check if stdout is a TTY
    return isatty(fileno(stdout));
#endif
}

// Format memory size
inline std::string formatMemory(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

// Format duration
inline std::string formatDuration(int seconds) {
    if (seconds < 60) {
        return std::to_string(seconds) + "s";
    } else if (seconds < 3600) {
        int mins = seconds / 60;
        int secs = seconds % 60;
        return std::to_string(mins) + "m " + std::to_string(secs) + "s";
    } else {
        int hours = seconds / 3600;
        int mins = (seconds % 3600) / 60;
        return std::to_string(hours) + "h " + std::to_string(mins) + "m";
    }
}

// Status badge
inline std::string statusBadge(const std::string& status) {
    bool useColors = shouldUseColors();

    if (status == "ACTIVE" || status == "Active") {
        if (useColors) {
            return Color::GREEN + "● ACTIVE" + Color::RESET;
        }
        return "● ACTIVE";
    } else if (status == "EXPIRED" || status == "Expired") {
        if (useColors) {
            return Color::RED + "● EXPIRED" + Color::RESET;
        }
        return "● EXPIRED";
    }
    return status;
}

// Print success message
inline void printSuccess(const std::string& msg) {
    if (shouldUseColors()) {
        std::cout << Color::GREEN << "✓ " << msg << Color::RESET << std::endl;
    } else {
        std::cout << "[OK] " << msg << std::endl;
    }
}

// Print error message
inline void printError(const std::string& msg) {
    if (shouldUseColors()) {
        std::cerr << Color::RED << "✗ " << msg << Color::RESET << std::endl;
    } else {
        std::cerr << "[ERROR] " << msg << std::endl;
    }
}

// Print warning message
inline void printWarning(const std::string& msg) {
    if (shouldUseColors()) {
        std::cout << Color::YELLOW << "⚠ " << msg << Color::RESET << std::endl;
    } else {
        std::cout << "[WARNING] " << msg << std::endl;
    }
}

// Print info message
inline void printInfo(const std::string& msg) {
    if (shouldUseColors()) {
        std::cout << Color::BLUE << "ℹ " << msg << Color::RESET << std::endl;
    } else {
        std::cout << "[INFO] " << msg << std::endl;
    }
}

// Simple table class
class Table {
   public:
    Table(const std::vector<std::string>& headers)
        : headers_(headers), useColors_(shouldUseColors()) {}

    void addRow(const std::vector<std::string>& row) { rows_.push_back(row); }

    void print() const {
        std::vector<size_t> widths = calculateWidths();

        // Print header
        printSeparator(widths, "+", "+", "+");
        printRow(headers_, widths, true);
        printSeparator(widths, "+", "+", "+");

        // Print rows
        for (size_t i = 0; i < rows_.size(); ++i) {
            printRow(rows_[i], widths, false);
            if (i < rows_.size() - 1) {
                printSeparator(widths, "+", "+", "+");
            }
        }

        // Print bottom
        printSeparator(widths, "+", "+", "+");
    }

   private:
    std::vector<std::string> headers_;
    std::vector<std::vector<std::string>> rows_;
    bool useColors_;

    std::vector<size_t> calculateWidths() const {
        std::vector<size_t> widths(headers_.size(), 0);

        // Check headers
        for (size_t i = 0; i < headers_.size(); ++i) {
            widths[i] = headers_[i].length();
        }

        // Check rows
        for (const auto& row : rows_) {
            for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
                widths[i] = std::max(widths[i], row[i].length());
            }
        }

        return widths;
    }

    void printSeparator(const std::vector<size_t>& widths, const std::string& left,
                        const std::string& mid, const std::string& right) const {
        std::cout << left;
        for (size_t i = 0; i < widths.size(); ++i) {
            std::cout << std::string(widths[i] + 2, '-');
            if (i < widths.size() - 1) {
                std::cout << mid;
            }
        }
        std::cout << right << std::endl;
    }

    void printRow(const std::vector<std::string>& row, const std::vector<size_t>& widths,
                  bool isHeader) const {
        std::cout << "|";

        for (size_t i = 0; i < widths.size(); ++i) {
            std::string cell = i < row.size() ? row[i] : "";

            if (useColors_ && isHeader) {
                std::cout << " " << Color::BOLD << Color::CYAN << std::left << std::setw(widths[i])
                          << cell << Color::RESET << " |";
            } else {
                std::cout << " " << std::left << std::setw(widths[i]) << cell << " |";
            }
        }
        std::cout << std::endl;
    }
};

}  // namespace cli
}  // namespace chronos

#endif  // CHRONOS_CLI_FORMATTER_H
