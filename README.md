# Chronos GPU Partitioner

![Chronos CI](https://github.com/oabraham1/chronos/workflows/Chronos%20CI/badge.svg)
![Docker](https://github.com/oabraham1/chronos/workflows/Docker/badge.svg)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

Chronos is a time-based GPU partitioning utility that allows multiple users or applications to share a single GPU by creating exclusive time-limited partitions. Built with OpenCL, it works across platforms including macOS (even on Apple Silicon), Linux, and Windows.

![Chronos Terminal Demo](docs/images/chronos-demo.gif)

## Features

- **Time-based partitioning**: Allocate GPU resources for specific durations
- **Exclusive access**: Prevent resource contention between users and applications
- **Cross-platform**: Works on macOS, Linux, and Windows
- **OpenCL-based**: Compatible with most GPU vendors (NVIDIA, AMD, Intel, Apple)
- **Memory allocation control**: Specify exact memory fractions for partitions
- **Command-line interface**: Simple and scriptable usage
- **C++ library API**: Embed Chronos functionality in your applications
- **Lock management**: Ensures exclusive access to allocated resources

## Installation

### Option 1: Build from Source

The recommended way to install the latest version with all features:

```bash
# Clone the repository
git clone https://github.com/oabraham1/chronos.git
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
```

#### Build with Tests and Examples (Optional)

```bash
# Configure with tests and examples
cmake -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..
make

# Run tests
ctest

# Run examples
./bin/simple_partition
./bin/advanced_usage
```

### Option 2: Pre-built Binaries

Download platform-specific binaries from the [Releases page](https://github.com/oabraham1/chronos/releases).

#### Linux

```bash
curl -L -o chronos-linux https://github.com/oabraham1/chronos/releases/latest/download/chronos-linux
chmod +x chronos-linux
sudo mv chronos-linux /usr/local/bin/chronos
```

#### macOS

```bash
curl -L -o chronos-macos https://github.com/oabraham1/chronos/releases/latest/download/chronos-macos
chmod +x chronos-macos
sudo mv chronos-macos /usr/local/bin/chronos
```

#### Windows

1. Download [chronos-windows.exe](https://github.com/oabraham1/chronos/releases/latest/download/chronos-windows.exe)
2. Move to a directory in your PATH or execute directly

### Option 3: Docker

```bash
# Pull from GitHub Container Registry
docker pull ghcr.io/oabraham1/chronos:latest

# Run commands
docker run --gpus all --rm ghcr.io/oabraham1/chronos:latest stats
```

See [Docker Usage Guide](docs/docker-usage.md) for more details on Docker integration.

### Option 4: Quick Installer Script

For Linux and macOS systems:

```bash
# Install with administrative privileges
curl -sSL https://raw.githubusercontent.com/oabraham1/chronos/main/install.sh | sudo bash

# Or install to user directory without sudo
curl -sSL https://raw.githubusercontent.com/oabraham1/chronos/main/install-user.sh | bash
```

## Usage

### Command-line Interface

```bash
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
```

### Library API Usage

```cpp
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
```

## Use Cases

### Shared GPU Resources in Multi-User Environments

```bash
# User 1: Train a model with 50% GPU for 2 hours
chronos create 0 0.5 7200

# User 2: Run visualization with 30% GPU for 1 hour
chronos create 0 0.3 3600
```

### Resource Guarantees for Critical Applications

```bash
# Ensure computer vision system has 40% GPU resources for 24 hours
chronos create 0 0.4 86400
```

### Testing with Limited Resources

```bash
# Test application with only 25% of GPU memory
chronos create 0 0.25 3600
```

## How It Works

Chronos uses OpenCL to detect available GPUs and manage memory partitioning. The key components:

1. **Device Detection**: OpenCL is used to discover available compute devices
2. **Memory Management**: Tracks and allocates memory fractions on a per-device basis
3. **Lock System**: Files in `/tmp/chronos_locks/` (or similar location) control exclusive access
4. **Time Management**: Background thread monitors durations and releases expired partitions

## Project Structure

The production-grade C++ structure of Chronos:

```
chronos/
├── include/                 # Public header files
│   ├── chronos.h            # Main public API
│   └── chronos_utils.h      # Utility functions
├── src/                     # Implementation files
│   ├── core/                # Core functionality
│   │   ├── device_info.h    # Device information structures
│   │   ├── device_info.cpp
│   │   ├── gpu_partition.h  # Partition data structures
│   │   └── gpu_partition.cpp
│   ├── platform/            # Platform-specific implementations
│   │   ├── platform.h       # Platform abstraction interface
│   │   ├── unix_platform.h
│   │   ├── unix_platform.cpp
│   │   ├── windows_platform.h
│   │   └── windows_platform.cpp
│   ├── utils/               # Internal utilities
│   │   ├── lock_file.h      # Lock file management
│   │   ├── lock_file.cpp
│   │   ├── time_utils.h     # Time-related utilities
│   │   └── time_utils.cpp
│   ├── partitioner.cpp      # Implementation of main partitioner
│   └── chronos_utils.cpp    # Utility function implementations
├── apps/                    # Application code
│   └── cli/                 # Command-line interface
│       ├── main.cpp         # CLI entry point
│       ├── commands.h       # Command handlers
│       └── commands.cpp
├── tests/                   # Unit and integration tests
│   ├── test_device_info.cpp
│   ├── test_lock_file.cpp
│   └── test_partitioner.cpp
├── examples/                # Example usage
│   ├── simple_partition.cpp
│   └── advanced_usage.cpp
├── CMakeLists.txt           # Main build file
└── README.md                # Project documentation
```

## Advanced Configuration

Custom configurations can be placed in:
- `/etc/chronos/config.json` (system-wide)
- `~/.config/chronos/config.json` (user-specific)

Example configuration:
```json
{
  "default_duration": 3600,
  "lock_directory": "/var/run/chronos/locks",
  "log_level": "info"
}
```

### Environment Variables

- `CHRONOS_LOCK_DIR`: Override the lock file directory
- `CHRONOS_LOG_LEVEL`: Set logging verbosity (debug, info, warn, error)
- `CHRONOS_DEFAULT_DEVICE`: Set the default device index

## Compatibility

Chronos has been tested on:

| Platform | GPU Vendors | Status |
|----------|-------------|--------|
| macOS 13+ (Intel) | Intel, AMD | ✅ |
| macOS 13+ (Apple Silicon) | Apple | ✅ |
| Ubuntu 20.04/22.04 | NVIDIA, AMD, Intel | ✅ |
| Windows 10/11 | NVIDIA, AMD, Intel | ✅ |

## Troubleshooting

### Common Issues

**No OpenCL devices found**:
- Ensure OpenCL drivers are installed for your GPU
- On macOS, this should work by default
- On Linux, install vendor-specific OpenCL packages

**Permission denied for lock files**:
```bash
sudo mkdir -p /tmp/chronos_locks
sudo chmod 777 /tmp/chronos_locks
```

**Unable to create partition**:
- Check available GPU memory with `chronos stats`
- Try a smaller memory fraction
- Make sure no conflicting partitions exist

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

1. Fork the repository
2. Create a feature branch: `git checkout -b my-new-feature`
3. Commit your changes: `git commit -am 'Add some feature'`
4. Push to the branch: `git push origin my-new-feature`
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact

- GitHub Issues: [https://github.com/oabraham1/chronos/issues](https://github.com/oabraham1/chronos/issues)
- Email: abrahamojima2018@gmail.com
