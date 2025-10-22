# Copyright (c) 2025 Ojima Abraham
# Licensed under the Business Source License 1.1
# Additional Use Grant: Production use permitted if gross revenue < $2,000/month
# Change Date: 2029-09-14, converts to GPL v3.0
# See LICENSE file for details

# benchmarks/CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# Add the benchmark executable
add_executable(benchmark_chronos benchmark_chronos.cpp)

# Link the benchmark against the chronos library
target_link_libraries(benchmark_chronos PRIVATE chronos)
