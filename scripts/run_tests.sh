#!/usr/bin/env bash
# Run test suite with different configurations

set -eo pipefail

TEST_BIN="./build/gpu_tests"
declare -a TEST_CASES=(
    "-p 10 -t 5"
    "-p 25 -t 3"
    "-p 50 -t 2"
)

run_test() {
    local config=$1
    echo "Running test: $config"
    $TEST_BIN $config || {
        echo "Test failed: $config"
        exit 1
    }
}

main() {
    # Run unit tests
    ./build/gpu_tests --gtest_filter=*Basic*
    
    # Run integration tests
    for config in "${TEST_CASES[@]}"; do
        run_test "$config"
    done
    
    echo "All tests passed successfully!"
}

main "$@"