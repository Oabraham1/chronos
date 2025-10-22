# Chronos Python Bindings

Python bindings for Chronos GPU Partitioner - Fair GPU time-sharing with automatic expiration.

## Installation

### From Source

```bash
# Clone the repository
git clone https://github.com/oabraham1/chronos
cd chronos

# Build and install
pip install -e .
```

### From PyPI (Coming Soon)

```bash
pip install chronos-gpu
```

## Quick Start

```python
from chronos import Partitioner

# Create a partitioner
partitioner = Partitioner()

# Request 50% of GPU 0 for 1 hour
with partitioner.create(device=0, memory=0.5, duration=3600) as partition:
    # Use your GPU here
    import torch
    model = torch.nn.Sequential(
        torch.nn.Linear(100, 50),
        torch.nn.ReLU(),
        torch.nn.Linear(50, 10)
    ).cuda()

    # Train your model
    # Partition automatically released when done or after 1 hour
```

## API Reference

### Partitioner

The main class for managing GPU partitions.

```python
partitioner = Partitioner()
```

#### Methods

##### `create(device, memory, duration, user=None)`

Create a new GPU partition.

**Parameters:**
- `device` (int): GPU device index (0, 1, 2, ...)
- `memory` (float): Fraction of GPU memory to allocate (0.0 - 1.0)
- `duration` (int): Duration in seconds
- `user` (str, optional): Username to assign partition to (admin only)

**Returns:** `Partition` object

**Example:**
```python
# Basic usage
partition = partitioner.create(device=0, memory=0.5, duration=3600)

# Create for another user (requires admin)
partition = partitioner.create(device=0, memory=0.3, duration=7200, user="alice")

# Context manager (recommended)
with partitioner.create(device=0, memory=0.5, duration=3600) as p:
    # Your GPU code here
    pass
```

##### `list()`

List all active partitions.

**Returns:** List of `PartitionInfo` objects

**Example:**
```python
partitions = partitioner.list()
for p in partitions:
    print(f"Partition {p.partition_id}: {p.memory_fraction*100}% memory")
    print(f"  Time remaining: {p.time_remaining_seconds}s")
    print(f"  Owner: {p.username}")
```

##### `release(partition_id)`

Release a partition early (before expiration).

**Parameters:**
- `partition_id` (str): ID of the partition to release

**Returns:** `bool` - True if successful

**Example:**
```python
partition = partitioner.create(device=0, memory=0.5, duration=3600)
# ... do work ...
partitioner.release(partition.partition_id)
```

##### `get_available(device)`

Get available memory percentage for a device.

**Parameters:**
- `device` (int): Device index

**Returns:** `float` - Percentage available (0-100)

**Example:**
```python
available = partitioner.get_available(device=0)
print(f"GPU 0 has {available:.1f}% available")
```

##### `show_stats()`

Display statistics for all devices.

**Example:**
```python
partitioner.show_stats()
```

### Partition

Represents an active GPU partition. Usually obtained from `Partitioner.create()`.

#### Properties

- `partition_id` (str): Unique partition identifier
- `device` (int): Device index
- `memory_fraction` (float): Allocated memory fraction
- `duration` (int): Total duration in seconds
- `time_remaining` (int): Seconds remaining until expiration

#### Methods

##### `release()`

Release the partition early.

**Example:**
```python
partition = partitioner.create(device=0, memory=0.5, duration=3600)
# ... do work ...
partition.release()
```

#### Context Manager

Partitions support context manager protocol for automatic cleanup:

```python
with partitioner.create(device=0, memory=0.5, duration=3600) as partition:
    # GPU code here
    pass
# Partition automatically released here
```

### PartitionInfo

Information about an active partition (returned by `list()`).

#### Attributes

- `partition_id` (str): Unique identifier
- `device_index` (int): Device index
- `memory_fraction` (float): Allocated memory (0.0-1.0)
- `duration_seconds` (int): Total duration
- `time_remaining_seconds` (int): Time left
- `username` (str): Owner username
- `process_id` (int): Owner process ID
- `active` (bool): Whether partition is active

### ChronosError

Exception raised when operations fail.

**Example:**
```python
from chronos import ChronosError

try:
    partition = partitioner.create(device=0, memory=1.5, duration=3600)
except ChronosError as e:
    print(f"Failed to create partition: {e}")
```

## Usage Examples

### Basic ML Training

```python
from chronos import Partitioner
import torch

partitioner = Partitioner()

with partitioner.create(device=0, memory=0.5, duration=3600) as p:
    print(f"Training with {p.memory_fraction*100}% of GPU {p.device}")

    model = torch.nn.Sequential(
        torch.nn.Linear(784, 128),
        torch.nn.ReLU(),
        torch.nn.Linear(128, 10)
    ).cuda()

    optimizer = torch.optim.Adam(model.parameters())

    for epoch in range(100):
        # Training loop
        if p.time_remaining < 300:  # Less than 5 minutes left
            print("Time almost up, saving checkpoint...")
            torch.save(model.state_dict(), "checkpoint.pt")
            break
```

### Multiple Partitions

```python
from chronos import Partitioner

partitioner = Partitioner()

# Check availability first
available = partitioner.get_available(device=0)
print(f"GPU 0: {available:.1f}% available")

# Create multiple small partitions
partitions = []
for i in range(3):
    p = partitioner.create(device=0, memory=0.1, duration=1800)
    partitions.append(p)
    print(f"Created partition {p.partition_id}")

# List all active
all_partitions = partitioner.list()
print(f"Total active partitions: {len(all_partitions)}")

# Clean up
for p in partitions:
    p.release()
```

### Error Handling

```python
from chronos import Partitioner, ChronosError

partitioner = Partitioner()

try:
    # Try to allocate too much memory
    partition = partitioner.create(device=0, memory=0.9, duration=3600)
except ChronosError as e:
    print(f"Failed: {e}")

    # Fall back to smaller allocation
    available = partitioner.get_available(device=0) / 100.0
    partition = partitioner.create(device=0, memory=available * 0.8, duration=3600)
    print(f"Allocated {partition.memory_fraction*100:.1f}% instead")
```

### Admin: Create for Other Users

```python
from chronos import Partitioner

# Must run as admin/root
partitioner = Partitioner()

# Allocate resources for team members
partitioner.create(device=0, memory=0.3, duration=28800, user="alice")
partitioner.create(device=0, memory=0.2, duration=28800, user="bob")
partitioner.create(device=0, memory=0.15, duration=28800, user="carol")

print("Team resources allocated!")
partitioner.show_stats()
```

## Platform Support

- **Linux**: Full support (NVIDIA, AMD, Intel GPUs)
- **macOS**: Full support (Apple Silicon, Intel, AMD)
- **Windows**: Full support (NVIDIA, AMD, Intel GPUs)

## Requirements

- Python 3.7+
- Chronos library installed (built from source or via pip)
- OpenCL-compatible GPU

## Troubleshooting

### "Could not find Chronos library"

Make sure the Chronos library is built and installed:

```bash
cd chronos
mkdir build && cd build
cmake ..
make
sudo make install
```

Or set `LD_LIBRARY_PATH` (Linux) or `DYLD_LIBRARY_PATH` (macOS):

```bash
export LD_LIBRARY_PATH=/path/to/chronos/build/lib:$LD_LIBRARY_PATH
```

### "No OpenCL devices found"

Ensure you have:
1. GPU drivers installed
2. OpenCL runtime installed
3. Permissions to access GPU

### "Permission denied"

For user-specific partitions, you need admin privileges:

```bash
sudo python your_script.py
```

## Contributing

Contributions welcome! See [CONTRIBUTING.md](../../CONTRIBUTING.md).

## License

Apache License 2.0 - See [LICENSE](../../LICENSE)

## Support

- **Issues**: [GitHub Issues](https://github.com/oabraham1/chronos/issues)
- **Docs**: [Full Documentation](https://github.com/oabraham1/chronos)
