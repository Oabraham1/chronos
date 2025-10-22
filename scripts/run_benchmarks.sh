#!/bin/bash
# run_benchmarks.sh - Build and run benchmarks

set -e

echo "=== Chronos Benchmark Suite ==="
echo ""

# Check if benchmarks are built
if [ ! -f "build/bin/benchmark_chronos" ]; then
    echo "Benchmarks not built. Building now..."
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON ..
    make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
    cd ..
fi

echo "Running benchmarks..."
echo ""

# Run benchmarks
./build/bin/benchmark_chronos

echo ""
echo "=== Benchmark Complete ==="
echo ""

# Show results if CSV was created
if [ -f "chronos_benchmark_results.csv" ]; then
    echo "Results saved to: chronos_benchmark_results.csv"
    echo ""
    echo "Summary:"
    cat chronos_benchmark_results.csv
fi
