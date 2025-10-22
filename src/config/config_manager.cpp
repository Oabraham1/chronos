/**
 * @file config_manager.cpp
 * @brief Implementation of configuration management system
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

#include "config/config_manager.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cstdlib>

namespace chronos {
namespace config {

namespace {

ConfigValue yamlToConfigValue(const YAML::Node& node) {
    if (node.IsNull()) {
        return ConfigValue();
    } else if (node.IsScalar()) {
        try {
            // Try to parse as int
            return ConfigValue(node.as<int>());
        } catch (...) {
            try {
                // Try to parse as float
                return ConfigValue(node.as<float>());
            } catch (...) {
                try {
                    // Try to parse as bool
                    std::string str = node.as<std::string>();
                    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                    if (str == "true" || str == "yes" || str == "on" || str == "1") {
                        return ConfigValue(true);
                    } else if (str == "false" || str == "no" || str == "off" || str == "0") {
                        return ConfigValue(false);
                    }
                    // Default to string
                    return ConfigValue(node.as<std::string>());
                } catch (...) {
                    return ConfigValue(node.as<std::string>());
                }
            }
        }
    } else if (node.IsSequence()) {
        std::vector<std::string> list;
        for (const auto& item : node) {
            list.push_back(item.as<std::string>());
        }
        return ConfigValue(list);
    }
    return ConfigValue();
}

void yamlToConfigSection(const YAML::Node& node, ConfigSection& section) {
    if (!node.IsMap()) return;

    for (const auto& it : node) {
        std::string key = it.first.as<std::string>();

        if (it.second.IsMap()) {
            ConfigSection subsection;
            yamlToConfigSection(it.second, subsection);
            section.setSection(key, subsection);
        } else {
            section.set(key, yamlToConfigValue(it.second));
        }
    }
}

YAML::Node configValueToYaml(const ConfigValue& value) {
    YAML::Node node;

    if (!value.has_value()) {
        return node;
    }

    try {
        node = value.get<int>();
        return node;
    } catch (...) {
    }

    try {
        node = value.get<float>();
        return node;
    } catch (...) {
    }

    try {
        node = value.get<bool>();
        return node;
    } catch (...) {
    }

    try {
        node = value.get<std::string>();
        return node;
    } catch (...) {
    }

    try {
        auto list = value.get<std::vector<std::string>>();
        for (const auto& item : list) {
            node.push_back(item);
        }
        return node;
    } catch (...) {
    }

    return node;
}

void configSectionToYaml(const ConfigSection& section, YAML::Node& node) {
    for (const auto& key : section.keys()) {
        node[key] = configValueToYaml(section.get(key));
    }
}

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;

    while (std::getline(ss, part, '.')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }

    return parts;
}

}  // anonymous namespace

ConfigValue ConfigSection::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second;
    }
    return ConfigValue();
}

std::string ConfigSection::getString(const std::string& key,
                                     const std::string& defaultValue) const {
    auto value = get(key);
    if (value.has_value()) {
        try {
            return value.get<std::string>();
        } catch (...) {
            try {
                return std::to_string(value.get<int>());
            } catch (...) {
            }
            try {
                return std::to_string(value.get<float>());
            } catch (...) {
            }
            try {
                return value.get<bool>() ? "true" : "false";
            } catch (...) {
            }
        }
    }
    return defaultValue;
}

int ConfigSection::getInt(const std::string& key, int defaultValue) const {
    auto value = get(key);
    if (value.has_value()) {
        try {
            return value.get<int>();
        } catch (...) {
            try {
                return std::stoi(value.get<std::string>());
            } catch (...) {
            }
        }
    }
    return defaultValue;
}

float ConfigSection::getFloat(const std::string& key, float defaultValue) const {
    auto value = get(key);
    if (value.has_value()) {
        try {
            return value.get<float>();
        } catch (...) {
            try {
                return std::stof(value.get<std::string>());
            } catch (...) {
            }
        }
    }
    return defaultValue;
}

bool ConfigSection::getBool(const std::string& key, bool defaultValue) const {
    auto value = get(key);
    if (value.has_value()) {
        try {
            return value.get<bool>();
        } catch (...) {
            try {
                std::string str = value.get<std::string>();
                std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                if (str == "true" || str == "yes" || str == "on" || str == "1") {
                    return true;
                } else if (str == "false" || str == "no" || str == "off" || str == "0") {
                    return false;
                }
            } catch (...) {
            }
        }
    }
    return defaultValue;
}

std::vector<std::string> ConfigSection::getStringList(const std::string& key) const {
    auto value = get(key);
    if (value.has_value()) {
        try {
            return value.get<std::vector<std::string>>();
        } catch (...) {
        }
    }
    return {};
}

std::vector<int> ConfigSection::getIntList(const std::string& key) const {
    auto stringList = getStringList(key);
    std::vector<int> intList;

    for (const auto& str : stringList) {
        try {
            intList.push_back(std::stoi(str));
        } catch (...) {
        }
    }

    return intList;
}

void ConfigSection::set(const std::string& key, const ConfigValue& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_[key] = value;
}

void ConfigSection::setString(const std::string& key, const std::string& value) {
    set(key, ConfigValue(value));
}

void ConfigSection::setInt(const std::string& key, int value) { set(key, ConfigValue(value)); }

void ConfigSection::setFloat(const std::string& key, float value) { set(key, ConfigValue(value)); }

void ConfigSection::setBool(const std::string& key, bool value) { set(key, ConfigValue(value)); }

bool ConfigSection::has(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return values_.find(key) != values_.end();
}

std::vector<std::string> ConfigSection::keys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto& pair : values_) {
        result.push_back(pair.first);
    }
    return result;
}

void ConfigSection::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    values_.clear();
    sections_.clear();
}

ConfigSection ConfigSection::getSection(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sections_.find(name);
    if (it != sections_.end()) {
        return it->second;
    }
    return ConfigSection();
}

void ConfigSection::setSection(const std::string& name, const ConfigSection& section) {
    std::lock_guard<std::mutex> lock(mutex_);
    sections_[name] = section;
}

class ConfigManager::Impl {
   public:
    Impl() : autoReload_(false), reloadThread_(nullptr) { loadDefaults(); }

    ~Impl() { stopAutoReload(); }

    void loadDefaults() {
        std::string defaultConfig = R"(
chronos:
  core:
    lock_directory: /tmp/chronos_locks
    state_directory: /var/lib/chronos
    max_partitions_per_gpu: 10

  logging:
    level: INFO
    output: console
    file_path: /var/log/chronos/chronos.log
    format: json
    max_file_size_mb: 100
    max_files: 10

  memory:
    enforce_limits: true
    oversubscription_ratio: 1.0
    allocation_granularity_mb: 256

  process:
    monitor_interval_seconds: 5
    cleanup_orphaned: true
    zombie_timeout_seconds: 60

  scheduler:
    type: fifo
    preemption_enabled: false

  api:
    enabled: false
    port: 8080
    host: 0.0.0.0
    auth_enabled: false
    api_key: ""
    rate_limit_per_minute: 60

  metrics:
    enabled: true
    collection_interval_seconds: 10
    prometheus_enabled: false
    prometheus_port: 9090

  gpu:
    exclude_devices: []
    prefer_devices: []

  python:
    enabled: true
    numpy_integration: true

  advanced:
    debug_mode: false
    enable_profiling: false
    command_timeout_seconds: 30
)";

        loadFromStringInternal(defaultConfig);
    }

    bool loadFromStringInternal(const std::string& yamlContent) {
        try {
            YAML::Node root = YAML::Load(yamlContent);
            yamlToConfigSection(root, rootSection_);
            notifyCallbacks("");
            return true;
        } catch (const YAML::Exception& e) {
            lastError_ = "YAML parse error: " + std::string(e.what());
            return false;
        }
    }

    void loadFromEnvironmentInternal(const std::string& prefix) {
        extern char** environ;

        for (char** env = environ; *env != nullptr; ++env) {
            std::string envStr(*env);
            size_t pos = envStr.find('=');

            if (pos != std::string::npos) {
                std::string key = envStr.substr(0, pos);
                std::string value = envStr.substr(pos + 1);

                if (key.find(prefix) == 0) {
                    std::string configPath = key.substr(prefix.length());
                    std::transform(configPath.begin(), configPath.end(), configPath.begin(),
                                   ::tolower);
                    std::replace(configPath.begin(), configPath.end(), '_', '.');

                    setValueByPath(configPath, value);
                }
            }
        }
    }

    ConfigValue getValueByPath(const std::string& path) const {
        auto parts = splitPath(path);
        if (parts.empty()) {
            return ConfigValue();
        }

        const ConfigSection* current = &rootSection_;

        for (size_t i = 0; i < parts.size() - 1; ++i) {
            ConfigSection next = current->getSection(parts[i]);
            if (!next.keys().empty()) {
                current = &next;
            } else {
                return ConfigValue();
            }
        }

        return current->get(parts.back());
    }

    void setValueByPath(const std::string& path, const std::string& value) {
        auto parts = splitPath(path);
        if (parts.empty()) return;

        ConfigSection* current = &rootSection_;

        for (size_t i = 0; i < parts.size() - 1; ++i) {
            ConfigSection section = current->getSection(parts[i]);
            if (section.keys().empty()) {
                current->setSection(parts[i], ConfigSection());
            }
            current = &sections_[parts[i]];
        }

        current->setString(parts.back(), value);
        notifyCallbacks(path);
    }

    void notifyCallbacks(const std::string& path) {
        std::lock_guard<std::mutex> lock(callbackMutex_);

        for (const auto& [handle, callback] : callbacks_) {
            if (callback.first.empty() || path.find(callback.first) == 0) {
                callback.second(path);
            }
        }
    }

    void startAutoReload() {
        if (autoReload_ && !configFilePath_.empty() && !reloadThread_) {
            reloadThread_ = std::make_unique<std::thread>([this]() {
                auto lastModified = std::filesystem::last_write_time(configFilePath_);

                while (autoReload_) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));

                    try {
                        auto currentModified = std::filesystem::last_write_time(configFilePath_);
                        if (currentModified != lastModified) {
                            lastModified = currentModified;
                            reload();
                        }
                    } catch (...) {
                    }
                }
            });
        }
    }

    void stopAutoReload() {
        autoReload_ = false;
        if (reloadThread_ && reloadThread_->joinable()) {
            reloadThread_->join();
            reloadThread_.reset();
        }
    }

    bool reload() {
        if (!configFilePath_.empty()) {
            std::ifstream file(configFilePath_);
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                return loadFromStringInternal(buffer.str());
            }
        }
        return false;
    }

    ConfigSection rootSection_;
    std::map<std::string, ConfigSection> sections_;
    std::string configFilePath_;
    std::atomic<bool> autoReload_;
    std::unique_ptr<std::thread> reloadThread_;

    std::map<size_t, std::pair<std::string, ConfigManager::ChangeCallback>> callbacks_;
    size_t nextCallbackHandle_ = 1;
    std::mutex callbackMutex_;

    std::string lastError_;
    mutable std::mutex mutex_;
};

ConfigManager::ConfigManager() : pImpl(std::make_unique<Impl>()) {}

ConfigManager::~ConfigManager() = default;

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    bool result = pImpl->loadFromStringInternal(buffer.str());
    if (result) {
        pImpl->configFilePath_ = filePath;
    }

    return result;
}

bool ConfigManager::loadFromString(const std::string& yamlContent) {
    return pImpl->loadFromStringInternal(yamlContent);
}

void ConfigManager::loadFromEnvironment(const std::string& prefix) {
    pImpl->loadFromEnvironmentInternal(prefix);
}

bool ConfigManager::saveToFile(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(pImpl->mutex_);

    try {
        YAML::Node root;
        configSectionToYaml(pImpl->rootSection_, root);

        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }

        file << root;
        return true;
    } catch (...) {
        return false;
    }
}

ConfigSection ConfigManager::getSection(const std::string& path) const {
    std::lock_guard<std::mutex> lock(pImpl->mutex_);

    auto parts = splitPath(path);
    if (parts.empty()) {
        return pImpl->rootSection_;
    }

    const ConfigSection* current = &pImpl->rootSection_;

    for (const auto& part : parts) {
        ConfigSection next = current->getSection(part);
        if (!next.keys().empty()) {
            current = &next;
        } else {
            return ConfigSection();
        }
    }

    return *current;
}

ConfigValue ConfigManager::get(const std::string& path) const {
    std::lock_guard<std::mutex> lock(pImpl->mutex_);
    return pImpl->getValueByPath(path);
}

std::string ConfigManager::getString(const std::string& path,
                                     const std::string& defaultValue) const {
    auto value = get(path);
    return value.has_value() ? value.get<std::string>(defaultValue) : defaultValue;
}

int ConfigManager::getInt(const std::string& path, int defaultValue) const {
    auto value = get(path);
    return value.has_value() ? value.get<int>(defaultValue) : defaultValue;
}

float ConfigManager::getFloat(const std::string& path, float defaultValue) const {
    auto value = get(path);
    return value.has_value() ? value.get<float>(defaultValue) : defaultValue;
}

bool ConfigManager::getBool(const std::string& path, bool defaultValue) const {
    auto value = get(path);
    return value.has_value() ? value.get<bool>(defaultValue) : defaultValue;
}

template <typename T>
void ConfigManager::set(const std::string& path, const T& value) {
    std::lock_guard<std::mutex> lock(pImpl->mutex_);
    pImpl->setValueByPath(path, std::to_string(value));
}

size_t ConfigManager::registerChangeCallback(const std::string& path, ChangeCallback callback) {
    std::lock_guard<std::mutex> lock(pImpl->callbackMutex_);
    size_t handle = pImpl->nextCallbackHandle_++;
    pImpl->callbacks_[handle] = {path, callback};
    return handle;
}

void ConfigManager::unregisterChangeCallback(size_t handle) {
    std::lock_guard<std::mutex> lock(pImpl->callbackMutex_);
    pImpl->callbacks_.erase(handle);
}

bool ConfigManager::reload() { return pImpl->reload(); }

void ConfigManager::reset() {
    std::lock_guard<std::mutex> lock(pImpl->mutex_);
    pImpl->rootSection_.clear();
    pImpl->sections_.clear();
    pImpl->loadDefaults();
}

bool ConfigManager::validate() const {
    std::vector<std::string> requiredPaths = {
        "chronos.core.lock_directory", "chronos.logging.level", "chronos.memory.enforce_limits"};

    for (const auto& path : requiredPaths) {
        if (!get(path).has_value()) {
            return false;
        }
    }

    return true;
}

std::vector<std::string> ConfigManager::getValidationErrors() const {
    std::vector<std::string> errors;

    if (!get("chronos.core.lock_directory").has_value()) {
        errors.push_back("Missing required field: chronos.core.lock_directory");
    }

    if (!get("chronos.logging.level").has_value()) {
        errors.push_back("Missing required field: chronos.logging.level");
    }

    return errors;
}

void ConfigManager::setConfigFilePath(const std::string& filePath) {
    pImpl->configFilePath_ = filePath;
}

void ConfigManager::setAutoReload(bool enable) {
    if (enable && !pImpl->autoReload_) {
        pImpl->autoReload_ = true;
        pImpl->startAutoReload();
    } else if (!enable && pImpl->autoReload_) {
        pImpl->stopAutoReload();
    }
}

std::string ConfigManager::toYamlString() const {
    std::lock_guard<std::mutex> lock(pImpl->mutex_);
    YAML::Node root;
    configSectionToYaml(pImpl->rootSection_, root);
    std::stringstream ss;
    ss << root;
    return ss.str();
}

void ConfigManager::merge(const ConfigSection& other, bool overwrite) {
    std::lock_guard<std::mutex> lock(pImpl->mutex_);
}

}  // namespace config
}  // namespace chronos
