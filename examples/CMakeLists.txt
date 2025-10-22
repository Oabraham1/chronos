# Copyright (c) 2025 Ojima Abraham
# Licensed under the Business Source License 1.1
# Additional Use Grant: Production use permitted if gross revenue < $2,000/month
# Change Date: 2029-09-14, converts to GPL v3.0
# See LICENSE file for details

cmake_minimum_required(VERSION 3.10)

# Simple partition example
add_executable(simple_partition simple_partition.cpp)
target_link_libraries(simple_partition chronos)

# Advanced usage example
add_executable(advanced_usage advanced_usage.cpp)
target_link_libraries(advanced_usage chronos)
