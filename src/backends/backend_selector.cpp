/**
 * @file backend_selector.cpp
 * @brief Backend selection implementation
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

#include "backends/backend_selector.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>

#include "backends/mps_backend.h"
#include "backends/opencl_backend.h"
#include "backends/rocm_backend.h"
#include "backends/stub_backend.h"

namespace chronos {
namespace backends {

namespace {

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

}  // anonymous namespace

bool BackendSelector::checkMPSAvailable() { return MPSBackend::checkAvailable(); }

bool BackendSelector::checkROCmAvailable() { return ROCmBackend::checkAvailable(); }

bool BackendSelector::checkOpenCLAvailable() { return OpenCLBackend::checkAvailable(); }

std::unique_ptr<ExecutionBackend> BackendSelector::selectBestBackend() {
    // 1. Check for environment variable override
    const char* envBackend = std::getenv("CHRONOS_BACKEND");
    if (envBackend) {
        std::string backendName = toLower(envBackend);
        auto backend = createBackend(backendName);
        if (backend && backend->initialize()) {
            std::cout << "Chronos: Using " << backend->getName()
                      << " backend (via CHRONOS_BACKEND env var)" << std::endl;
            return backend;
        }
        std::cerr << "Warning: Requested backend '" << envBackend
                  << "' not available, falling back to auto-detection" << std::endl;
    }

    // 2. Try NVIDIA MPS (true concurrent execution)
    if (checkMPSAvailable()) {
        auto backend = std::make_unique<MPSBackend>();
        if (backend->initialize()) {
            std::cout << "Chronos: Using NVIDIA MPS backend (concurrent execution enabled)"
                      << std::endl;
            return backend;
        }
    }

    // 3. Try AMD ROCm
    if (checkROCmAvailable()) {
        auto backend = std::make_unique<ROCmBackend>();
        if (backend->initialize()) {
            std::cout << "Chronos: Using ROCm backend (AMD GPU support)" << std::endl;
            return backend;
        }
    }

    // 4. Fall back to OpenCL (universal cross-vendor support)
    if (checkOpenCLAvailable()) {
        auto backend = std::make_unique<OpenCLBackend>();
        if (backend->initialize()) {
            std::cout << "Chronos: Using OpenCL backend (cross-vendor, time-sliced)" << std::endl;
            return backend;
        }
    }

    // 5. Last resort: Stub backend
    std::cerr << "Warning: No GPU backend available, using stub backend" << std::endl;
    auto backend = std::make_unique<StubBackend>();
    if (backend->initialize()) {
        return backend;
    }

    return nullptr;
}

std::unique_ptr<ExecutionBackend> BackendSelector::createBackend(const std::string& name) {
    std::string lowerName = toLower(name);

    if (lowerName == "mps" || lowerName == "nvidia" || lowerName == "nvidia_mps") {
        return std::make_unique<MPSBackend>();
    }

    if (lowerName == "rocm" || lowerName == "amd") {
        return std::make_unique<ROCmBackend>();
    }

    if (lowerName == "opencl" || lowerName == "cl") {
        return std::make_unique<OpenCLBackend>();
    }

    if (lowerName == "stub" || lowerName == "test") {
        return std::make_unique<StubBackend>();
    }

    return nullptr;
}

std::vector<BackendInfo> BackendSelector::listAvailableBackends() {
    std::vector<BackendInfo> backends;

    // NVIDIA MPS
    {
        BackendInfo info;
        info.name = "mps";
        info.description = "NVIDIA MPS (true concurrent execution)";
        info.mode = ExecutionMode::CONCURRENT;
        info.available = checkMPSAvailable();
        info.priority = 100;
        backends.push_back(info);
    }

    // ROCm
    {
        BackendInfo info;
        info.name = "rocm";
        info.description = "AMD ROCm (AMD GPU support)";
        info.mode = ExecutionMode::TIME_SLICED;
        info.available = checkROCmAvailable();
        info.priority = 80;
        backends.push_back(info);
    }

    // OpenCL
    {
        BackendInfo info;
        info.name = "opencl";
        info.description = "OpenCL (cross-vendor, time-sliced)";
        info.mode = ExecutionMode::TIME_SLICED;
        info.available = checkOpenCLAvailable();
        info.priority = 50;
        backends.push_back(info);
    }

    // Stub
    {
        BackendInfo info;
        info.name = "stub";
        info.description = "Stub (no GPU, testing only)";
        info.mode = ExecutionMode::STUB;
        info.available = true;
        info.priority = 0;
        backends.push_back(info);
    }

    return backends;
}

bool BackendSelector::hasGPUSupport() {
    return checkMPSAvailable() || checkROCmAvailable() || checkOpenCLAvailable();
}

bool BackendSelector::hasConcurrentSupport() {
    return checkMPSAvailable();  // Currently only MPS provides true concurrent execution
}

std::string BackendSelector::getRecommendedBackend() {
    if (checkMPSAvailable()) return "mps";
    if (checkROCmAvailable()) return "rocm";
    if (checkOpenCLAvailable()) return "opencl";
    return "stub";
}

}  // namespace backends
}  // namespace chronos
