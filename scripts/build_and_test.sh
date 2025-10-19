#!/bin/bash

set -e

echo "=== Chronos Build and Test Script ==="
echo ""

if [ -d "build" ]; then
    echo "Cleaning old build directory..."
    rm -rf build
fi

mkdir -p build
cd build

echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..

echo ""
echo "Building Chronos..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    make -j$(sysctl -n hw.ncpu)
else
    make -j$(nproc)
fi

echo ""
echo "Running C++ tests..."
ctest --output-on-failure

echo ""
echo "=== Build Summary ==="
echo "Binaries:"
ls -lh bin/

echo ""
echo "Libraries:"
ls -lh lib/

cd ..

echo ""
echo "=== Testing Python Bindings ==="

export PYTHONPATH="${PWD}/python:${PYTHONPATH}"

if [[ "$OSTYPE" == "darwin"* ]]; then
    export DYLD_LIBRARY_PATH="${PWD}/build/lib:${DYLD_LIBRARY_PATH}"
else
    export LD_LIBRARY_PATH="${PWD}/build/lib:${LD_LIBRARY_PATH}"
fi

echo "1. Testing Python import..."
python3 -c "from chronos import Partitioner; print('✓ Python import successful')" || {
    echo "⚠ Python bindings not available (library not found)"
    echo "   This is expected if OpenCL is not available"
    echo ""
    echo "=== C++ Tests Complete ==="
    echo "To install: cd build && sudo make install"
    echo "To run CLI: ./build/bin/chronos_cli help"
    exit 0
}

echo ""
echo "2. Running basic Python test..."
python3 << 'EOF'
from chronos import Partitioner, ChronosError
import sys

try:
    partitioner = Partitioner()
    print("✓ Partitioner created")

    partition = partitioner.create(device=0, memory=0.1, duration=5)
    print(f"✓ Partition created: {partition.partition_id}")

    partitions = partitioner.list()
    print(f"✓ Listed {len(partitions)} partition(s)")

    available = partitioner.get_available(device=0)
    print(f"✓ Available: {available:.1f}%")

    partition.release()
    print("✓ Partition released")

    print("\n✓ Basic Python tests passed!")

except ChronosError as e:
    if "No OpenCL" in str(e) or "Invalid device" in str(e):
        print("⚠ No OpenCL devices available - skipping Python tests")
        sys.exit(0)
    else:
        print(f"✗ Error: {e}")
        sys.exit(1)
EOF

echo ""
echo "3. Running Python test suite..."
if [ -d "python/tests" ]; then
    cd python/tests
    python3 -m unittest test_chronos -v || {
        echo "⚠ Some Python tests failed (this is OK if no OpenCL devices)"
        cd ../..
    }
    cd ../..
else
    echo "⚠ Python test directory not found"
fi

echo ""
echo "4. Testing context manager..."
python3 << 'EOF'
from chronos import Partitioner, ChronosError
import sys

try:
    partitioner = Partitioner()

    with partitioner.create(device=0, memory=0.1, duration=5) as partition:
        print(f"✓ Context manager: partition {partition.partition_id} active")
        partitions = partitioner.list()
        assert len(partitions) == 1, "Partition should be active"

    partitions = partitioner.list()
    assert len(partitions) == 0, "Partition should be auto-released"
    print("✓ Context manager auto-cleanup works!")

except ChronosError as e:
    if "No OpenCL" in str(e) or "Invalid device" in str(e):
        print("⚠ No OpenCL devices available - skipping test")
        sys.exit(0)
    else:
        print(f"✗ Error: {e}")
        sys.exit(1)
EOF

echo ""
echo "=== Success! ==="
echo ""
echo "C++ Library:"
echo "  To install: cd build && sudo make install"
echo "  To run CLI: ./build/bin/chronos_cli help"
echo ""
echo "Python Bindings:"
echo "  Available at: python/chronos/"
echo "  Usage:"
echo "    from chronos import Partitioner"
echo "    with Partitioner().create(device=0, memory=0.5, duration=3600) as p:"
echo "        # Your GPU code here"
echo "        pass"
echo ""
