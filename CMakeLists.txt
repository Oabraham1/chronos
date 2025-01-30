cmake_minimum_required(VERSION 3.12)
project(GPUPartitioner LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

find_package(CUDAToolkit REQUIRED)

add_executable(gpu_partitioner
    src/main.cpp
    src/gpu_partitioner.cpp
    src/gpu_session.cpp
)

target_include_directories(gpu_partitioner
    PRIVATE include
)

target_link_libraries(gpu_partitioner
    PRIVATE CUDA::cudart
)

# Testing configuration
option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(gpu_tests tests/test_gpu_partitioner.cpp src/gpu_partitioner.cpp src/gpu_session.cpp)
    target_link_libraries(gpu_tests PRIVATE CUDA::cudart GTest::GTest GTest::Main)
    add_test(NAME gpu_tests COMMAND gpu_tests)
endif()
