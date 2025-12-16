/**
 * @file backend_selector.h
 * @brief Backend selection and management
 *
 * The BackendSelector chooses the best available execution backend:
 * 1. NVIDIA MPS - If NVIDIA GPU with MPS support detected (true concurrency)
 * 2. ROCm - If AMD GPU with ROCm detected
 * 3. OpenCL - Universal fallback for ALL GPU vendors
 *
 * IMPORTANT: OpenCL support is ALWAYS maintained for cross-vendor compatibility.
 * The enhanced backends (MPS, ROCm) are optional upgrades, not replacements.
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

#ifndef CHRONOS_BACKEND_SELECTOR_H
#define CHRONOS_BACKEND_SELECTOR_H

#include <memory>
#include <string>
#include <vector>

#include "backends/execution_backend.h"

namespace chronos {
namespace backends {

/**
 * @struct BackendInfo
 * @brief Information about an available backend
 */
struct BackendInfo {
    std::string name;         ///< Backend name
    std::string description;  ///< Human-readable description
    ExecutionMode mode;       ///< Execution mode
    bool available;           ///< Whether backend is available
    int priority;             ///< Selection priority (higher = preferred)
};

/**
 * @class BackendSelector
 * @brief Selects and manages GPU execution backends
 *
 * Selection priority:
 * 1. Check CHRONOS_BACKEND environment variable for override
 * 2. Try NVIDIA MPS (true concurrent execution)
 * 3. Try AMD ROCm
 * 4. Fall back to OpenCL (universal cross-vendor support)
 * 5. Fall back to Stub (for testing/no-GPU environments)
 */
class BackendSelector {
   public:
    /**
     * @brief Select the best available backend
     *
     * This method will:
     * 1. Check for CHRONOS_BACKEND environment variable override
     * 2. Try backends in priority order (MPS > ROCm > OpenCL > Stub)
     * 3. Return the first backend that initializes successfully
     *
     * @return Unique pointer to initialized backend, or nullptr on failure
     */
    static std::unique_ptr<ExecutionBackend> selectBestBackend();

    /**
     * @brief Create a specific backend by name
     *
     * Valid names: "mps", "rocm", "opencl", "stub"
     *
     * @param name Backend name (case-insensitive)
     * @return Unique pointer to backend, or nullptr if not available
     */
    static std::unique_ptr<ExecutionBackend> createBackend(const std::string& name);

    /**
     * @brief List all available backends
     * @return Vector of backend information structures
     */
    static std::vector<BackendInfo> listAvailableBackends();

    /**
     * @brief Check if any GPU backend is available
     * @return true if at least OpenCL or better is available
     */
    static bool hasGPUSupport();

    /**
     * @brief Check if concurrent execution is possible
     * @return true if MPS or ROCm is available
     */
    static bool hasConcurrentSupport();

    /**
     * @brief Get the recommended backend for this system
     * @return Backend name string
     */
    static std::string getRecommendedBackend();

   private:
    /**
     * @brief Check if NVIDIA MPS is available
     */
    static bool checkMPSAvailable();

    /**
     * @brief Check if AMD ROCm is available
     */
    static bool checkROCmAvailable();

    /**
     * @brief Check if OpenCL is available
     */
    static bool checkOpenCLAvailable();
};

}  // namespace backends
}  // namespace chronos

#endif  // CHRONOS_BACKEND_SELECTOR_H
