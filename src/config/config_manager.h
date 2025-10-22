/**
 * @file config_manager.h
 * @brief Configuration management system for Chronos
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

#ifndef CHRONOS_CONFIG_MANAGER_H
#define CHRONOS_CONFIG_MANAGER_H

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace chronos {
namespace config {

/**
 * @class ConfigValue
 * @brief Represents a configuration value with type safety
 */
class ConfigValue {
   public:
    ConfigValue() = default;
    explicit ConfigValue(const std::any& value) : value_(value) {}

    template <typename T>
    T get() const {
        try {
            return std::any_cast<T>(value_);
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Configuration type mismatch: " + std::string(e.what()));
        }
    }

    template <typename T>
    T get(const T& defaultValue) const {
        try {
            return std::any_cast<T>(value_);
        } catch (...) {
            return defaultValue;
        }
    }

    bool has_value() const { return value_.has_value(); }

   private:
    std::any value_;
};

/**
 * @class ConfigSection
 * @brief Represents a section of configuration
 */
class ConfigSection {
   public:
    ConfigSection() = default;

    ConfigValue get(const std::string& key) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    std::vector<std::string> getStringList(const std::string& key) const;
    std::vector<int> getIntList(const std::string& key) const;

    void set(const std::string& key, const ConfigValue& value);
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setBool(const std::string& key, bool value);

    bool has(const std::string& key) const;
    std::vector<std::string> keys() const;
    void clear();

    ConfigSection getSection(const std::string& name) const;
    void setSection(const std::string& name, const ConfigSection& section);

   private:
    std::map<std::string, ConfigValue> values_;
    std::map<std::string, ConfigSection> sections_;
    mutable std::mutex mutex_;
};

/**
 * @class ConfigManager
 * @brief Main configuration manager class
 *
 * Manages application configuration from multiple sources:
 * - Default configuration
 * - Configuration files (YAML)
 * - Environment variables
 * - Runtime updates
 */
class ConfigManager {
   public:
    /**
     * @brief Get singleton instance
     */
    static ConfigManager& getInstance();

    /**
     * @brief Load configuration from file
     * @param filePath Path to YAML configuration file
     * @return True if successful, false otherwise
     */
    bool loadFromFile(const std::string& filePath);

    /**
     * @brief Load configuration from string
     * @param yamlContent YAML content as string
     * @return True if successful, false otherwise
     */
    bool loadFromString(const std::string& yamlContent);

    /**
     * @brief Load environment variables
     * @param prefix Prefix for environment variables (e.g., "CHRONOS_")
     */
    void loadFromEnvironment(const std::string& prefix = "CHRONOS_");

    /**
     * @brief Save current configuration to file
     * @param filePath Path to save configuration
     * @return True if successful, false otherwise
     */
    bool saveToFile(const std::string& filePath) const;

    /**
     * @brief Get configuration section
     * @param path Dot-separated path (e.g., "core.logging.level")
     * @return Configuration section
     */
    ConfigSection getSection(const std::string& path) const;

    /**
     * @brief Get configuration value
     * @param path Dot-separated path to value
     * @return Configuration value
     */
    ConfigValue get(const std::string& path) const;

    /**
     * @brief Get string value
     * @param path Dot-separated path to value
     * @param defaultValue Default value if not found
     * @return String value
     */
    std::string getString(const std::string& path, const std::string& defaultValue = "") const;

    /**
     * @brief Get integer value
     * @param path Dot-separated path to value
     * @param defaultValue Default value if not found
     * @return Integer value
     */
    int getInt(const std::string& path, int defaultValue = 0) const;

    /**
     * @brief Get float value
     * @param path Dot-separated path to value
     * @param defaultValue Default value if not found
     * @return Float value
     */
    float getFloat(const std::string& path, float defaultValue = 0.0f) const;

    /**
     * @brief Get boolean value
     * @param path Dot-separated path to value
     * @param defaultValue Default value if not found
     * @return Boolean value
     */
    bool getBool(const std::string& path, bool defaultValue = false) const;

    /**
     * @brief Set configuration value
     * @param path Dot-separated path to value
     * @param value Value to set
     */
    template <typename T>
    void set(const std::string& path, const T& value);

    /**
     * @brief Register callback for configuration changes
     * @param path Path to watch (empty for all changes)
     * @param callback Callback function
     * @return Handle to unregister callback
     */
    using ChangeCallback = std::function<void(const std::string& path)>;
    size_t registerChangeCallback(const std::string& path, ChangeCallback callback);

    /**
     * @brief Unregister change callback
     * @param handle Handle returned by registerChangeCallback
     */
    void unregisterChangeCallback(size_t handle);

    /**
     * @brief Reload configuration from file
     * @return True if successful, false otherwise
     */
    bool reload();

    /**
     * @brief Reset to default configuration
     */
    void reset();

    /**
     * @brief Validate configuration against schema
     * @return True if valid, false otherwise
     */
    bool validate() const;

    /**
     * @brief Get validation errors
     * @return List of validation error messages
     */
    std::vector<std::string> getValidationErrors() const;

    /**
     * @brief Set configuration file path for auto-reload
     * @param filePath Path to configuration file
     */
    void setConfigFilePath(const std::string& filePath);

    /**
     * @brief Enable/disable auto-reload on file change
     * @param enable True to enable, false to disable
     */
    void setAutoReload(bool enable);

    /**
     * @brief Get current configuration as YAML string
     * @return YAML representation of configuration
     */
    std::string toYamlString() const;

    /**
     * @brief Merge configuration from another source
     * @param other Configuration to merge
     * @param overwrite True to overwrite existing values
     */
    void merge(const ConfigSection& other, bool overwrite = true);

   private:
    ConfigManager();
    ~ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @class ConfigSchema
 * @brief Schema validation for configuration
 */
class ConfigSchema {
   public:
    /**
     * @brief Define required fields
     */
    void addRequired(const std::string& path, const std::string& type);

    /**
     * @brief Define optional fields with defaults
     */
    template <typename T>
    void addOptional(const std::string& path, const T& defaultValue);

    /**
     * @brief Add validation rule
     */
    using ValidationRule = std::function<bool(const ConfigValue&)>;
    void addValidation(const std::string& path, ValidationRule rule,
                       const std::string& errorMessage);

    /**
     * @brief Validate configuration against schema
     */
    bool validate(const ConfigManager& config, std::vector<std::string>& errors) const;

    /**
     * @brief Get default configuration based on schema
     */
    ConfigSection getDefaults() const;

   private:
    struct Field {
        std::string path;
        std::string type;
        bool required;
        ConfigValue defaultValue;
        std::vector<std::pair<ValidationRule, std::string>> validations;
    };

    std::vector<Field> fields_;
};

/**
 * @brief Global configuration accessor
 */
inline ConfigManager& config() { return ConfigManager::getInstance(); }

}  // namespace config
}  // namespace chronos

#endif  // CHRONOS_CONFIG_MANAGER_H
