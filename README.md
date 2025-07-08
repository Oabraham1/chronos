# Chronos: A Time-Based GPU Partitioning System

![Chronos CI](https://github.com/oabraham1/chronos/workflows/Chronos%20CI/badge.svg)
![Docker](https://github.com/oabraham1/chronos/workflows/Docker/badge.svg)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

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

## Future Work

1. **Persistent Partitions**: Support for partitions surviving system reboots
2. **Priority Scheduling**: Implement priority-based partition allocation
3. **GPU Migration**: Live partition migration between devices
4. **Performance Isolation**: Ensure computational isolation between partitions

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Contact

- **Author**: Ojima Abraham
- **Email**: abrahamojima2018@gmail.com
- **GitHub**: https://github.com/oabraham1/chronos
