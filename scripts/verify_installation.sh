#!/usr/bin/env bash
# System verification script

set -eo pipefail

check_cuda() {
    echo "Checking CUDA installation..."
    nvcc --version | grep -q "release 12" || {
        echo "ERROR: CUDA 12.x not found!"
        exit 1
    }
}

check_build() {
    echo "Building project..."
    mkdir -p build && cd build
    cmake .. && make
    cd ..
}

run_test() {
    echo "Running functional test..."
    ./build/gpu_partitioner -p 5 -t 2 || {
        echo "ERROR: Runtime test failed!"
        exit 1
    }
}

main() {
    check_cuda
    check_build
    run_test
    echo "All system checks passed!"
}

main "$@"