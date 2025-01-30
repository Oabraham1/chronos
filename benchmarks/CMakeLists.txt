# benchmarks/CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# Add the benchmark executable
add_executable(benchmark_chronos benchmark_chronos.cpp)

# Link the benchmark against the chronos library
target_link_libraries(benchmark_chronos PRIVATE chronos)
