/**
 * @file logger.h
 * @brief Structured logging system for Chronos
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

#ifndef CHRONOS_LOGGER_H
#define CHRONOS_LOGGER_H

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace chronos {
namespace logging {

/**
 * @enum LogLevel
 * @brief Log severity levels
 */
enum class LogLevel { TRACE = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };

/**
 * @class LogContext
 * @brief Contextual information for log entries
 */
class LogContext {
   public:
    std::string file;
    int line;
    std::string function;
    std::thread::id threadId;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> fields;

    LogContext() : line(0), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @class LogSink
 * @brief Abstract base class for log output destinations
 */
class LogSink {
   public:
    virtual ~LogSink() = default;

    /**
     * @brief Write a log entry
     * @param level Log level
     * @param message Log message
     * @param context Additional context
     */
    virtual void write(LogLevel level, const std::string& message, const LogContext& context) = 0;

    /**
     * @brief Flush any buffered output
     */
    virtual void flush() = 0;

    /**
     * @brief Set minimum log level for this sink
     * @param level Minimum level to output
     */
    void setMinLevel(LogLevel level) { minLevel_ = level; }

    /**
     * @brief Check if level should be logged
     * @param level Level to check
     * @return True if should be logged
     */
    bool shouldLog(LogLevel level) const { return level >= minLevel_; }

   protected:
    LogLevel minLevel_ = LogLevel::TRACE;
};

/**
 * @class ConsoleSink
 * @brief Log output to console (stdout/stderr)
 */
class ConsoleSink : public LogSink {
   public:
    ConsoleSink(bool useColors = true, bool logToStderr = false);

    void write(LogLevel level, const std::string& message, const LogContext& context) override;
    void flush() override;

   private:
    bool useColors_;
    bool logToStderr_;
};

/**
 * @class FileSink
 * @brief Log output to file with rotation support
 */
class FileSink : public LogSink {
   public:
    FileSink(const std::string& filename, size_t maxFileSize = 100 * 1024 * 1024,
             size_t maxFiles = 10);
    ~FileSink();

    void write(LogLevel level, const std::string& message, const LogContext& context) override;
    void flush() override;

   private:
    void rotateIfNeeded();

    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @class JsonSink
 * @brief Log output in JSON format
 */
class JsonSink : public LogSink {
   public:
    JsonSink(std::shared_ptr<LogSink> underlyingSink);

    void write(LogLevel level, const std::string& message, const LogContext& context) override;
    void flush() override;

   private:
    std::shared_ptr<LogSink> underlyingSink_;
};

/**
 * @class Logger
 * @brief Main logger class
 */
class Logger {
   public:
    /**
     * @brief Get singleton instance
     */
    static Logger& getInstance();

    /**
     * @brief Configure logger from configuration manager
     */
    void configure();

    /**
     * @brief Add a log sink
     * @param sink Log sink to add
     */
    void addSink(std::shared_ptr<LogSink> sink);

    /**
     * @brief Remove all log sinks
     */
    void clearSinks();

    /**
     * @brief Set global log level
     * @param level Minimum level to log
     */
    void setLevel(LogLevel level);

    /**
     * @brief Get current log level
     * @return Current log level
     */
    LogLevel getLevel() const;

    /**
     * @brief Check if level would be logged
     * @param level Level to check
     * @return True if would be logged
     */
    bool shouldLog(LogLevel level) const;

    /**
     * @brief Log a message
     * @param level Log level
     * @param message Message to log
     * @param context Optional context
     */
    void log(LogLevel level, const std::string& message, const LogContext& context = LogContext());

    /**
     * @brief Flush all sinks
     */
    void flush();

    /**
     * @brief Set a context field for all future logs in this thread
     * @param key Field key
     * @param value Field value
     */
    void setThreadContext(const std::string& key, const std::string& value);

    /**
     * @brief Clear thread context
     */
    void clearThreadContext();

    /**
     * @brief Enable/disable performance tracking
     * @param enable True to enable
     */
    void setPerformanceTracking(bool enable);

   private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @class LogMessage
 * @brief RAII log message builder
 */
class LogMessage {
   public:
    LogMessage(LogLevel level, const char* file, int line, const char* function);
    ~LogMessage();

    /**
     * @brief Stream operator for building message
     */
    template <typename T>
    LogMessage& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

    /**
     * @brief Add structured field
     * @param key Field key
     * @param value Field value
     */
    LogMessage& withField(const std::string& key, const std::string& value);

    /**
     * @brief Add multiple fields
     * @param fields Map of fields
     */
    LogMessage& withFields(const std::map<std::string, std::string>& fields);

    /**
     * @brief Add error information
     * @param error Error code or exception
     */
    LogMessage& withError(const std::exception& error);
    LogMessage& withError(int errorCode);

    /**
     * @brief Add timing information
     * @param duration Duration to log
     */
    template <typename Duration>
    LogMessage& withDuration(const Duration& duration) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        return withField("duration_ms", std::to_string(ms.count()));
    }

   private:
    LogLevel level_;
    LogContext context_;
    std::ostringstream stream_;
};

/**
 * @class ScopedTimer
 * @brief RAII timer for performance logging
 */
class ScopedTimer {
   public:
    ScopedTimer(const std::string& name, LogLevel level = LogLevel::DEBUG);
    ~ScopedTimer();

    /**
     * @brief Add context field
     */
    ScopedTimer& withField(const std::string& key, const std::string& value);

   private:
    std::string name_;
    LogLevel level_;
    std::chrono::high_resolution_clock::time_point start_;
    std::map<std::string, std::string> fields_;
};

// Convenience functions
std::string levelToString(LogLevel level);
LogLevel stringToLevel(const std::string& str);

// Global logger accessor
inline Logger& logger() { return Logger::getInstance(); }

// Logging macros
#define LOG_TRACE()                                                                     \
    if (chronos::logging::logger().shouldLog(chronos::logging::LogLevel::TRACE))        \
    chronos::logging::LogMessage(chronos::logging::LogLevel::TRACE, __FILE__, __LINE__, \
                                 __FUNCTION__)

#define LOG_DEBUG()                                                                     \
    if (chronos::logging::logger().shouldLog(chronos::logging::LogLevel::DEBUG))        \
    chronos::logging::LogMessage(chronos::logging::LogLevel::DEBUG, __FILE__, __LINE__, \
                                 __FUNCTION__)

#define LOG_INFO()                                                              \
    if (chronos::logging::logger().shouldLog(chronos::logging::LogLevel::INFO)) \
    chronos::logging::LogMessage(chronos::logging::LogLevel::INFO, __FILE__, __LINE__, __FUNCTION__)

#define LOG_WARN()                                                              \
    if (chronos::logging::logger().shouldLog(chronos::logging::LogLevel::WARN)) \
    chronos::logging::LogMessage(chronos::logging::LogLevel::WARN, __FILE__, __LINE__, __FUNCTION__)

#define LOG_ERROR()                                                                     \
    if (chronos::logging::logger().shouldLog(chronos::logging::LogLevel::ERROR))        \
    chronos::logging::LogMessage(chronos::logging::LogLevel::ERROR, __FILE__, __LINE__, \
                                 __FUNCTION__)

#define LOG_FATAL()                                                                     \
    chronos::logging::LogMessage(chronos::logging::LogLevel::FATAL, __FILE__, __LINE__, \
                                 __FUNCTION__)

// Performance logging macros
#define LOG_TIMER(name) chronos::logging::ScopedTimer _timer_##__LINE__(name)

#define LOG_TIMER_WITH_LEVEL(name, level) \
    chronos::logging::ScopedTimer _timer_##__LINE__(name, level)

}  // namespace logging
}  // namespace chronos

#endif  // CHRONOS_LOGGER_H
