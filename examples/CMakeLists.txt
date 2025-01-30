cmake_minimum_required(VERSION 3.10)

# Simple partition example
add_executable(simple_partition simple_partition.cpp)
target_link_libraries(simple_partition chronos)

# Advanced usage example
add_executable(advanced_usage advanced_usage.cpp)
target_link_libraries(advanced_usage chronos)
