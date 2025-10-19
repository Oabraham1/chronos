/**
 * @file test_config_manager.cpp
 * @brief Unit tests for ConfigManager class
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

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#include "config/config_manager.h"

using namespace chronos::config;

/**
 * @brief Test loading configuration from string
 */
int testLoadFromString() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    std::string yamlContent = R"(
test:
  string_value: hello
  int_value: 42
  float_value: 3.14
  bool_value: true
  list_value:
    - item1
    - item2
    - item3
  nested:
    inner_value: nested_test
)";

    assert(config.loadFromString(yamlContent));

    // Test string value
    assert(config.getString("test.string_value") == "hello");

    // Test int value
    assert(config.getInt("test.int_value") == 42);

    // Test float value
    float floatVal = config.getFloat("test.float_value");
    assert(floatVal > 3.13f && floatVal < 3.15f);

    // Test bool value
    assert(config.getBool("test.bool_value") == true);

    // Test nested value
    assert(config.getString("test.nested.inner_value") == "nested_test");

    // Test default values for non-existent keys
    assert(config.getString("test.nonexistent", "default") == "default");
    assert(config.getInt("test.nonexistent", 99) == 99);
    assert(config.getBool("test.nonexistent", false) == false);

    std::cout << "testLoadFromString PASSED" << std::endl;
    return 0;
}

/**
 * @brief Test loading configuration from file
 */
int testLoadFromFile() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    // Create a temporary config file
    std::string tempFile = "/tmp/test_chronos_config.yaml";
    std::ofstream file(tempFile);
    file << R"(
chronos:
  test:
    file_value: from_file
    number: 123
)";
    file.close();

    assert(config.loadFromFile(tempFile));
    assert(config.getString("chronos.test.file_value") == "from_file");
    assert(config.getInt("chronos.test.number") == 123);

    // Clean up
    std::filesystem::remove(tempFile);

    std::cout << "testLoadFromFile PASSED" << std::endl;
    return 0;
}

/**
 * @brief Test environment variable override
 */
int testEnvironmentOverride() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    // Set environment variables
    setenv("CHRONOS_CORE_LOCK_DIRECTORY", "/custom/lock/dir", 1);
    setenv("CHRONOS_LOGGING_LEVEL", "DEBUG", 1);
    setenv("CHRONOS_MEMORY_ENFORCE_LIMITS", "false", 1);

    // Load from environment
    config.loadFromEnvironment("CHRONOS_");

    assert(config.getString("core.lock.directory") == "/custom/lock/dir");
    assert(config.getString("logging.level") == "DEBUG");
    assert(config.getBool("memory.enforce.limits") == false);

    // Clean up environment
    unsetenv("CHRONOS_CORE_LOCK_DIRECTORY");
    unsetenv("CHRONOS_LOGGING_LEVEL");
    unsetenv("CHRONOS_MEMORY_ENFORCE_LIMITS");

    std::cout << "testEnvironmentOverride PASSED" << std::endl;
    return 0;
}

/**
 * @brief Test configuration sections
 */
int testConfigSections() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    std::string yamlContent = R"(
chronos:
  logging:
    level: INFO
    output: console
    format: json
)";

    assert(config.loadFromString(yamlContent));

    // Get a section
    ConfigSection loggingSection = config.getSection("chronos.logging");

    // Test section values
    assert(loggingSection.getString("level") == "INFO");
    assert(loggingSection.getString("output") == "console");
    assert(loggingSection.getString("format") == "json");

    // Test section keys
    auto keys = loggingSection.keys();
    assert(keys.size() == 3);
    assert(std::find(keys.begin(), keys.end(), "level") != keys.end());
    assert(std::find(keys.begin(), keys.end(), "output") != keys.end());
    assert(std::find(keys.begin(), keys.end(), "format") != keys.end());

    std::cout << "testConfigSections PASSED" << std::endl;
    return 0;
}

/**
 * @brief Test configuration validation
 */
int testValidation() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    // Load default configuration (should be valid)
    assert(config.validate());

    // Load incomplete configuration
    std::string incompleteYaml = R"(
test:
  incomplete: true
)";

    config.loadFromString(incompleteYaml);

    // Should fail validation due to missing required fields
    assert(!config.validate());

    // Check validation errors
    auto errors = config.getValidationErrors();
    assert(!errors.empty());

    std::cout << "testValidation PASSED" << std::endl;
    return 0;
}

/**
 * @brief Test configuration callbacks
 */
int testCallbacks() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    bool callbackCalled = false;
    std::string changedPath;

    // Register callback
    size_t handle = config.registerChangeCallback(
        "test", [&callbackCalled, &changedPath](const std::string& path) {
            callbackCalled = true;
            changedPath = path;
        });

    // Trigger callback by changing configuration
    config.set("test.value", "new_value");

    // Give callback time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    assert(callbackCalled);
    assert(changedPath == "test.value");

    // Unregister callback
    config.unregisterChangeCallback(handle);

    // Reset for next test
    callbackCalled = false;
    config.set("test.another", "value");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Callback should not be called after unregistering
    assert(!callbackCalled);

    std::cout << "testCallbacks PASSED" << std::endl;
    return 0;
}

/**
 * @brief Test saving configuration to file
 */
int testSaveToFile() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    // Set some values
    config.set("test.save.string", "saved_value");
    config.set("test.save.number", 42);
    config.set("test.save.boolean", true);

    // Save to file
    std::string tempFile = "/tmp/test_chronos_save.yaml";
    assert(config.saveToFile(tempFile));

    // Reset and reload
    config.reset();
    assert(config.loadFromFile(tempFile));

    // Verify values were saved and loaded correctly
    assert(config.getString("test.save.string") == "saved_value");
    assert(config.getInt("test.save.number") == 42);
    assert(config.getBool("test.save.boolean") == true);

    // Clean up
    std::filesystem::remove(tempFile);

    std::cout << "testSaveToFile PASSED" << std::endl;
    return 0;
}

/**
 * @brief Test auto-reload functionality
 */
int testAutoReload() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    // Create initial config file
    std::string tempFile = "/tmp/test_chronos_autoreload.yaml";
    std::ofstream file(tempFile);
    file << R"(
test:
  value: initial
)";
    file.close();

    // Load and enable auto-reload
    assert(config.loadFromFile(tempFile));
    config.setConfigFilePath(tempFile);
    config.setAutoReload(true);

    // Verify initial value
    assert(config.getString("test.value") == "initial");

    // Wait a bit for file watcher to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Modify the file
    file.open(tempFile);
    file << R"(
test:
  value: updated
)";
    file.close();

    // Wait for auto-reload to detect change
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Verify value was auto-reloaded
    assert(config.getString("test.value") == "updated");

    // Disable auto-reload
    config.setAutoReload(false);

    // Clean up
    std::filesystem::remove(tempFile);

    std::cout << "testAutoReload PASSED" << std::endl;
    return 0;
}

/**
 * @brief Test default values
 */
int testDefaultValues() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    // Test that default configuration is loaded
    assert(config.getString("chronos.core.lock_directory") == "/tmp/chronos_locks");
    assert(config.getString("chronos.logging.level") == "INFO");
    assert(config.getBool("chronos.memory.enforce_limits") == true);
    assert(config.getFloat("chronos.memory.oversubscription_ratio") == 1.0f);
    assert(config.getInt("chronos.process.monitor_interval_seconds") == 5);

    std::cout << "testDefaultValues PASSED" << std::endl;
    return 0;
}

/**
 * @brief Test list values
 */
int testListValues() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    std::string yamlContent = R"(
test:
  string_list:
    - first
    - second
    - third
  int_list:
    - 1
    - 2
    - 3
)";

    assert(config.loadFromString(yamlContent));

    // Get string list through section
    ConfigSection section = config.getSection("test");
    auto stringList = section.getStringList("string_list");
    assert(stringList.size() == 3);
    assert(stringList[0] == "first");
    assert(stringList[1] == "second");
    assert(stringList[2] == "third");

    // Get int list
    auto intList = section.getIntList("int_list");
    assert(intList.size() == 3);
    assert(intList[0] == 1);
    assert(intList[1] == 2);
    assert(intList[2] == 3);

    std::cout << "testListValues PASSED" << std::endl;
    return 0;
}

/**
 * @brief Test thread safety
 */
int testThreadSafety() {
    ConfigManager& config = ConfigManager::getInstance();
    config.reset();

    const int numThreads = 10;
    const int numOperations = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);

    // Launch multiple threads performing concurrent operations
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&config, &successCount, i, numOperations]() {
            for (int j = 0; j < numOperations; ++j) {
                std::string key = "test.thread" + std::to_string(i) + ".value" + std::to_string(j);
                config.set(key, std::to_string(i * 1000 + j));

                std::string value = config.getString(key);
                if (value == std::to_string(i * 1000 + j)) {
                    successCount++;
                }

                // Also test reading default values
                config.getString("chronos.logging.level");
                config.getInt("chronos.process.monitor_interval_seconds");
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // All operations should have succeeded
    assert(successCount == numThreads * numOperations);

    std::cout << "testThreadSafety PASSED" << std::endl;
    return 0;
}

/**
 * @brief Main test runner
 */
int main() {
    std::cout << "==== Testing ConfigManager ====" << std::endl;

    int result = 0;

    std::cout << "\n1. Testing Load from String:" << std::endl;
    result += testLoadFromString();

    std::cout << "\n2. Testing Load from File:" << std::endl;
    result += testLoadFromFile();

    std::cout << "\n3. Testing Environment Override:" << std::endl;
    result += testEnvironmentOverride();

    std::cout << "\n4. Testing Config Sections:" << std::endl;
    result += testConfigSections();

    std::cout << "\n5. Testing Validation:" << std::endl;
    result += testValidation();

    std::cout << "\n6. Testing Callbacks:" << std::endl;
    result += testCallbacks();

    std::cout << "\n7. Testing Save to File:" << std::endl;
    result += testSaveToFile();

    std::cout << "\n8. Testing Auto-Reload:" << std::endl;
    result += testAutoReload();

    std::cout << "\n9. Testing Default Values:" << std::endl;
    result += testDefaultValues();

    std::cout << "\n10. Testing List Values:" << std::endl;
    result += testListValues();

    std::cout << "\n11. Testing Thread Safety:" << std::endl;
    result += testThreadSafety();

    std::cout << "\n==== Test Summary ====" << std::endl;
    if (result == 0) {
        std::cout << "All tests PASSED!" << std::endl;
    } else {
        std::cout << result << " tests FAILED." << std::endl;
    }

    return result;
}
