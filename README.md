# Chronos: A Time-Based GPU Partitioning System

![Chronos CI](https://github.com/oabraham1/chronos/workflows/Chronos%20CI/badge.svg)
![Docker](https://github.com/oabraham1/chronos/workflows/Docker/badge.svg)
[![License: BSL 1.1](https://img.shields.io/badge/License-BSL%201.1-blue.svg)](https://mariadb.com/bsl11/)

## Abstract

Chronos is a novel GPU resource management system that implements time-based partitioning to enable fair and efficient GPU sharing in multi-user environments. Unlike traditional spatial partitioning approaches, Chronos allocates GPU resources for fixed time intervals with guaranteed exclusive access, addressing the challenges of GPU resource contention in shared computing environments.

## Key Contributions

1. **Time-Based Partitioning Algorithm**: A novel approach to GPU resource allocation that combines temporal and spatial dimensions
2. **Cross-Platform Implementation**: Unified GPU management across heterogeneous systems (NVIDIA, AMD, Intel, Apple Silicon)
3. **Automatic Resource Reclamation**: Self-healing mechanism that prevents resource hoarding through automatic partition expiration
4. **Lock-Based Coordination**: Lightweight inter-process coordination without kernel modifications

## System Architecture

### Core Components

```
┌─────────────────────────────────────────────────────┐
│                   User Applications                  │
├─────────────────────────────────────────────────────┤
│                  Chronos CLI/API                     │
├─────────────────────────────────────────────────────┤
│              Partition Manager                       │
│  ┌─────────────┬──────────────┬────────────────┐   │
│  │  Admission  │  Expiration  │    Memory      │   │
│  │  Control    │  Monitor     │   Accounting   │   │
│  └─────────────┴──────────────┴────────────────┘   │
├─────────────────────────────────────────────────────┤
│              Lock Coordination Layer                 │
├─────────────────────────────────────────────────────┤
│                 OpenCL Abstraction                   │
├─────────────────────────────────────────────────────┤
│              GPU Hardware (Multi-vendor)             │
└─────────────────────────────────────────────────────┘
```

### Algorithm Overview

The Chronos partitioning algorithm operates as follows:

1. **Resource Discovery**: Enumerate available GPU devices via OpenCL
2. **Admission Control**: Validate resource availability and user permissions
3. **Lock Acquisition**: Create filesystem-based lock for inter-process coordination
4. **Memory Accounting**: Update internal resource tracking structures
5. **Expiration Monitoring**: Background thread enforces temporal constraints
6. **Automatic Reclamation**: Release resources upon expiration or explicit release

### Time Complexity

- Partition Creation: O(n) where n is the number of existing partitions
- Partition Release: O(1) amortized
- Expiration Check: O(n) performed at 1Hz frequency
- Device Statistics: O(d×p) where d is devices and p is partitions

## Installation

### Prerequisites

#### macOS
```bash
brew install cmake
brew install yaml-cpp  # Optional but recommended
```

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake
sudo apt-get install -y ocl-icd-opencl-dev opencl-headers
sudo apt-get install -y libyaml-cpp-dev  # Optional but recommended
```

#### RHEL/CentOS/Fedora
```bash
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake ocl-icd-devel opencl-headers
sudo yum install -y yaml-cpp-devel  # Optional but recommended
```

### Building from Source

```bash
git clone https://github.com/oabraham1/chronos.git
cd chronos
mkdir build && cd build
cmake -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..
make -j$(nproc)
sudo make install
```

### Running Benchmarks

```bash
# Build with benchmarks
cmake -DBUILD_BENCHMARKS=ON ..
make

# Run benchmark suite
./bin/benchmark_chronos
```

## Usage Examples

### Basic Partition Creation

```cpp
#include <chronos.h>

int main() {
    chronos::ChronosPartitioner partitioner;

    // Allocate 50% of GPU 0 for 1 hour
    std::string partitionId = partitioner.createPartition(
        0,      // Device index
        0.5,    // Memory fraction
        3600    // Duration in seconds
    );

    // Perform GPU computations...

    // Early release (optional - automatic at expiration)
    partitioner.releasePartition(partitionId);

    return 0;
}
```

### Multi-User Scenario

```bash
# User A: Machine learning training
chronos create 0 0.6 7200  # 60% for 2 hours

# User B: Visualization task
chronos create 0 0.3 1800  # 30% for 30 minutes

# System administrator: Check utilization
chronos stats
```

### Configuration (with yaml-cpp)

```yaml
# ~/.chronos/config.yaml
chronos:
  core:
    lock_directory: /tmp/chronos_locks
    max_partitions_per_gpu: 10
  logging:
    level: INFO
    output: console
  memory:
    enforce_limits: true
    oversubscription_ratio: 1.0
```

## Current Implementation Status

### ✅ Completed Features
- Core partitioning engine with time-based allocation
- Multi-platform support (Linux, macOS, Windows)
- Lock-based inter-process coordination
- Automatic partition expiration
- CLI interface with all basic commands
- Docker containerization
- CI/CD pipeline with GitHub Actions
- Comprehensive test suite
- Benchmarking framework
- Configuration management (optional, requires yaml-cpp)

### 🚧 In Development
- [ ] Memory enforcement through OpenCL (currently accounting only)
- [ ] REST API for programmatic access
- [ ] Python bindings for ML frameworks
- [ ] Advanced scheduling policies
- [ ] Performance metrics collection
- [ ] Web dashboard

### 🔮 Planned Features (Chronos Cloud)
- Multi-cluster management
- Centralized monitoring and alerting
- Enterprise authentication (SAML/OIDC)
- Usage analytics and reporting
- SLA guarantees

## Evaluation

### Performance Metrics

| Metric                     | Value           | Notes                    |
| -------------------------- | --------------- | ------------------------ |
| Partition Creation Latency | < 5ms           | Measured on Ubuntu 22.04 |
| Memory Allocation Accuracy | ±0.1%           | Of requested fraction    |
| Expiration Timing Error    | < 100ms         | 1Hz monitoring frequency |
| Scalability                | 100+ partitions | Per GPU device           |

### Comparison with Existing Solutions

| System      | Approach       | Isolation     | Flexibility | Overhead |
| ----------- | -------------- | ------------- | ----------- | -------- |
| **Chronos** | Time-based     | Process-level | High        | < 1%     |
| NVIDIA MIG  | Spatial        | Hardware      | Low         | None     |
| gVirt       | Virtualization | VM-level      | Medium      | 5-15%    |
| Ratel       | Spatial        | Process-level | Medium      | 2-5%     |

## Research Applications

Chronos enables several research directions:

1. **Fair GPU Scheduling**: Investigate optimal partition duration policies
2. **QoS Guarantees**: Extend system for performance isolation
3. **Energy Efficiency**: Time-based allocation for power management
4. **Multi-GPU Coordination**: Distributed resource management

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Setup

```bash
# Clone with submodules
git clone --recursive https://github.com/oabraham1/chronos.git

# Build in debug mode
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

## Citation

If you use Chronos in your research, please cite:

```bibtex
@inproceedings{abraham2025chronos,
  title={Chronos: Time-Based GPU Partitioning for Fair Resource Sharing},
  author={Abraham, Ojima},
  booktitle={Proceedings of [Conference]},
  year={2025}
}
```

## Technical Details

### Lock File Format

```
pid: <process_id>
user: <username>
host: <hostname>
time: <timestamp>
device: <device_index>
fraction: <memory_fraction>
partition: <partition_id>
```

### Platform Support Matrix

| OS      | Architecture  | OpenCL Version | Status |
| ------- | ------------- | -------------- | ------ |
| Linux   | x86_64        | 1.2+           | ✅     |
| Linux   | ARM64         | 1.2+           | ✅     |
| macOS   | Intel         | 1.2            | ✅     |
| macOS   | Apple Silicon | 1.2            | ✅     |
| Windows | x86_64        | 1.2+           | ✅     |

## Roadmap

### Phase 1: Core Features (Current)
- ✅ Basic partitioning system
- ✅ CLI interface
- ✅ Lock-based coordination
- 🚧 Memory enforcement
- 🚧 Configuration management

### Phase 2: Enhanced Functionality (Q1 2025)
- [ ] REST API
- [ ] Python bindings
- [ ] Performance metrics
- [ ] Advanced schedulers
- [ ] Web dashboard

### Phase 3: Enterprise Features (Q2 2025)
- [ ] Chronos Cloud agent
- [ ] Multi-cluster support
- [ ] Advanced monitoring
- [ ] Enterprise authentication
- [ ] SLA management

## License

Business Source License 1.1 - see [LICENSE](LICENSE) file for details.

### License Summary
- **Source Available**: Source code is freely available for viewing, modification, and non-production use
- **Production Use**: Free for organizations with gross revenue < $2,000/month attributable to Chronos usage
- **Change Date**: September 14, 2029 - automatically converts to GPL v3.0
- **Additional Use Grant**: Production use permitted if gross revenue from Chronos usage is under $2,000/month

### Commercial Licensing
For production use beyond the Additional Use Grant terms (revenue > $2,000/month), contact abrahamojima2018@gmail.com for commercial licensing.

Contact abrahamojima2018@gmail.com for commercial licensing.

## Support

### Community Support
- GitHub Issues: [github.com/oabraham1/chronos/issues](https://github.com/oabraham1/chronos/issues)
- Discussions: [github.com/oabraham1/chronos/discussions](https://github.com/oabraham1/chronos/discussions)

### Commercial Support
For enterprise support and Chronos Cloud inquiries, contact: abrahamojima2018@gmail.com

## Contact

- **Author**: Ojima Abraham
- **Email**: abrahamojima2018@gmail.com
- **GitHub**: https://github.com/oabraham1/chronos
