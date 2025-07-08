Chronos GPU Partitioner
Chronos is a time-based GPU partitioning utility that allows multiple users or applications to share a single GPU by creating exclusive time-limited partitions. Built with OpenCL, it works across platforms including macOS (even on Apple Silicon), Linux, and Windows.

📖 Abstract
Chronos is a user-level GPU partitioner that enables time-based sharing of GPU resources. It allows multiple users or applications to run on a single GPU without interfering with each other by creating exclusive, time-limited partitions. Chronos is implemented in C++ using OpenCL for cross-platform compatibility and provides both a command-line interface and a C++ library for integration into other applications.

✨ Key Contributions
Temporal GPU Partitioning: A novel approach to GPU sharing that allocates resources for a specific duration.

Cross-Platform: The first GPU partitioner that works on macOS, Linux, and Windows with a variety of GPU vendors.

Low Overhead: A lightweight design that introduces minimal performance overhead.

User-Level Implementation: Does not require any kernel modifications or special hardware support.

🚀 Installation
Option 1: Build from Source
The recommended way to install the latest version with all features:

# Clone the repository

git clone [https://github.com/oabraham1/chronos.git](https://github.com/oabraham1/chronos.git)
cd chronos

# Create build directory

mkdir build && cd build

# Configure and build

cmake ..
make

# Install (may require sudo)

sudo make install

# Verify installation

chronos stats

Option 2: Docker 🐳

# Pull from GitHub Container Registry

docker pull ghcr.io/oabraham1/chronos:latest

# Run commands

docker run --gpus all --rm ghcr.io/oabraham1/chronos:latest stats

See Docker Usage Guide for more details on Docker integration.

Option 3: Quick Installer Script
For Linux and macOS systems:

# Install with administrative privileges

curl -sSL [https://raw.githubusercontent.com/oabraham1/chronos/main/install.sh](https://raw.githubusercontent.com/oabraham1/chronos/main/install.sh) | sudo bash

# Or install to user directory without sudo

curl -sSL [https://raw.githubusercontent.com/oabraham1/chronos/main/install-user.sh](https://raw.githubusercontent.com/oabraham1/chronos/main/install-user.sh) | bash

💻 Usage
Command-line Interface

# Show help

chronos help

# View device statistics

chronos stats

# Create a partition (50% of GPU 0 for 1 hour)

chronos create 0 0.5 3600

# List active partitions

chronos list

# Release a partition early

chronos release partition_0001

# Check available memory percentage

chronos available 0

Library API Usage
#include <chronos.h>
#include <iostream>

int main() {
// Create partitioner instance
chronos::ChronosPartitioner partitioner;

    // Show available devices
    partitioner.showDeviceStats();

    // Create a partition (30% of GPU 0 for 10 minutes)
    std::string partitionId = partitioner.createPartition(0, 0.3, 600);

    if (!partitionId.empty()) {
        std::cout << "Created partition: " << partitionId << std::endl;

        // Use the GPU for your work...

        // Release the partition early when done
        partitioner.releasePartition(partitionId);
    }

    return 0;

}

🔬 Benchmarking & Evaluation
To support academic evaluation, this project includes a comprehensive benchmark suite. For details on the partitioning algorithm and its theoretical properties, see the formal algorithm description.

To build and run the benchmarks:

# From the project root directory

mkdir -p build && cd build
cmake .. -DBUILD_BENCHMARKS=ON
make
./bin/benchmark_chronos

A Python script is also provided to automate running the benchmarks and plotting the results.

# From the project root directory

python3 benchmarks/run_experiments.py

🛠️ How It Works
Chronos uses OpenCL to detect available GPUs and manage memory partitioning. The key components:

Device Detection: OpenCL is used to discover available compute devices

Memory Management: Tracks and allocates memory fractions on a per-device basis

Lock System: Files in /tmp/chronos_locks/ (or similar location) control exclusive access

Time Management: Background thread monitors durations and releases expired partitions

✅ Compatibility
Platform

GPU Vendors

Status

macOS 13+ (Intel)

Intel, AMD

✅

macOS 13+ (Apple Silicon)

Apple

✅

Ubuntu 20.04/22.04

NVIDIA, AMD, Intel

✅

Windows 10/11

NVIDIA, AMD, Intel

✅

🤔 Troubleshooting
Common Issues
No OpenCL devices found:

Ensure OpenCL drivers are installed for your GPU

On macOS, this should work by default

On Linux, install vendor-specific OpenCL packages

Permission denied for lock files:

sudo mkdir -p /tmp/chronos_locks
sudo chmod 777 /tmp/chronos_locks

Unable to create partition:

Check available GPU memory with chronos stats

Try a smaller memory fraction

Make sure no conflicting partitions exist

🙌 Contributing
Contributions are welcome! Please see CONTRIBUTING.md for guidelines.

📜 License
This project is licensed under the MIT License - see the LICENSE file for details.

📧 Contact
GitHub Issues: https://github.com/oabraham1/chronos/issues

Email: abrahamojima2018@gmail.com
