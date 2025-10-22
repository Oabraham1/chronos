# Chronos User Guide

Complete guide to using Chronos GPU Partitioner for fair time-based GPU sharing.

## Table of Contents

1. [Introduction](#introduction)
2. [Core Concepts](#core-concepts)
3. [Getting Started](#getting-started)
4. [CLI Reference](#cli-reference)
5. [Python API](#python-api)
6. [Use Cases](#use-cases)
7. [Best Practices](#best-practices)
8. [Troubleshooting](#troubleshooting)

---

## Introduction

Chronos is a GPU time-sharing system that provides:

- **Fair allocation** through time-based partitions
- **Automatic expiration** to prevent resource hogging
- **User isolation** with permission controls
- **Memory enforcement** to prevent overuse
- **Cross-platform** support (Linux, macOS, Windows)

### When to Use Chronos

✅ **Good fit:**
- Shared GPU workstations in research labs
- Small teams with limited GPU resources
- Development environments with multiple projects
- Educational settings with many students

❌ **Not ideal for:**
- Production ML inference (use model serving frameworks)
- Real-time applications (use dedicated GPUs)
- Enterprise-scale clusters (use Kubernetes + device plugins)

---

## Core Concepts

### Partitions

A **partition** is a time-limited allocation of GPU memory to a specific user.

```
Partition = {
    device: GPU device index (0, 1, 2, ...)
    memory: Fraction of GPU memory (0.0 - 1.0)
    duration: Time limit in seconds
    user: Owner username
    start_time: When partition was created
}
```

**Key properties:**
- Automatically expires after duration
- Only owner can release early
- Prevents overlapping allocations
- Enforces memory limits

### Time-Based Sharing

Unlike traditional resource managers, Chronos uses **time** as the primary allocation unit:

```
Traditional: "User A gets 50% of GPU forever"
Chronos:     "User A gets 50% of GPU for 1 hour"
```

This ensures:
- No indefinite resource hoarding
- Predictable access times
- Automatic cleanup
- Fair rotation

### User Isolation

Each partition is owned by a specific user:

```bash
# Alice creates a partition
$ chronos create 0 0.5 3600
Created partition_0001 for alice

# Bob cannot release Alice's partition
$ chronos release partition_0001
Permission denied: partition owned by alice
```

---

## Getting Started

### Step 1: Check GPU Availability

```bash
# Show all GPUs and their status
chronos stats
```

Output:
```
Device statistics:
=================
Device 0: NVIDIA GeForce RTX 3080
  Type: GPU
  Vendor: NVIDIA Corporation
  OpenCL version: OpenCL 3.0
  Memory:
    Total: 10240 MB
    Used: 0 MB
    Available: 10240 MB
    Usage: 0.00%
  Chronos management:
    Active partitions: 0
```

### Step 2: Check Current Availability

```bash
# Check percentage available on GPU 0
chronos available 0
```

Output: `100.00` (100% available)

### Step 3: Create Your First Partition

```bash
# Request 50% of GPU 0 for 1 hour (3600 seconds)
chronos create 0 0.5 3600
```

Output:
```
Created partition partition_0001 on device 0 (NVIDIA GeForce RTX 3080)
with 5120 MB for 3600 seconds
Assigned to user: alice (Created by: alice, PID: 12345)
```

### Step 4: Verify Partition

```bash
# List all active partitions
chronos list
```

Output:
```
Active partitions:
-----------------
ID: partition_0001
  Device: 0 (NVIDIA GeForce RTX 3080)
  Memory: 50%
  Time remaining: 3598 seconds
  Owner: alice (PID: 12345)
```

### Step 5: Use Your GPU

Now you can run your GPU workload with guaranteed access to 50% of GPU memory.

### Step 6: Release or Wait

```bash
# Option 1: Release when done (before expiration)
chronos release partition_0001

# Option 2: Wait for automatic expiration (after 3600 seconds)
# Partition automatically released, no action needed
```

---

## CLI Reference

### `chronos create`

Create a new GPU partition.

```bash
chronos create <device> <memory> <duration> [--user <username>]
```

**Parameters:**
- `device` (int): GPU device index (0, 1, 2, ...)
- `memory` (float): Memory fraction (0.0 - 1.0)
- `duration` (int): Duration in seconds
- `--user` (string, optional): Target user (admin only)

**Examples:**
```bash
# 50% of GPU 0 for 1 hour
chronos create 0 0.5 3600

# 30% of GPU 1 for 8 hours
chronos create 1 0.3 28800

# Create for another user (requires sudo)
sudo chronos create 0 0.5 3600 --user alice
```

**Common durations:**
- 5 minutes: `300`
- 30 minutes: `1800`
- 1 hour: `3600`
- 4 hours: `14400`
- 8 hours: `28800`
- 24 hours: `86400`

### `chronos list`

List all active partitions.

```bash
chronos list
```

**Output shows:**
- Partition ID
- Device index and name
- Memory allocation (%)
- Time remaining
- Owner username and PID
- Memory usage (if available)

### `chronos release`

Release a partition early (before expiration).

```bash
chronos release <partition_id>
```

**Example:**
```bash
chronos release partition_0001
```

**Note:** Only the partition owner can release it (or admin with sudo).

### `chronos stats`

Show detailed statistics for all GPU devices.

```bash
chronos stats
```

**Output includes:**
- Device name and type
- Vendor and OpenCL version
- Total/used/available memory
- Number of active partitions

### `chronos available`

Get available memory percentage for a specific device.

```bash
chronos available <device>
```

**Example:**
```bash
chronos available 0
```

Output: `75.00` (75% available)

**Useful for scripting:**
```bash
available=$(chronos available 0)
if (( $(echo "$available > 50" | bc -l) )); then
    echo "Enough memory available"
    chronos create 0 0.5 3600
fi
```

### `chronos help`

Show help message with usage examples.

```bash
chronos help
```

---

## Python API

### Basic Usage

```python
from chronos import Partitioner

# Create partitioner
partitioner = Partitioner()

# Create partition (returns Partition object)
partition = partitioner.create(device=0, memory=0.5, duration=3600)

print(f"Created: {partition.partition_id}")
print(f"Using {partition.memory_fraction*100}% of GPU {partition.device}")

# Use your GPU here
import torch
model = torch.nn.Sequential(...).cuda()

# Release when done
partition.release()
```

### Context Manager (Recommended)

```python
from chronos import Partitioner

partitioner = Partitioner()

# Automatically releases on exit
with partitioner.create(device=0, memory=0.5, duration=3600) as p:
    print(f"Partition: {p.partition_id}")

    # Your GPU code here
    import torch
    model = torch.nn.Sequential(...).cuda()
    # Train model...

# Partition automatically released here
```

### List Partitions

```python
partitions = partitioner.list()

for p in partitions:
    print(f"ID: {p.partition_id}")
    print(f"  Memory: {p.memory_fraction*100}%")
    print(f"  Time left: {p.time_remaining_seconds}s")
    print(f"  Owner: {p.username}")
```

### Check Availability

```python
available = partitioner.get_available(device=0)
print(f"GPU 0: {available:.1f}% available")

if available > 50:
    partition = partitioner.create(device=0, memory=0.5, duration=3600)
```

### Error Handling

```python
from chronos import Partitioner, ChronosError

partitioner = Partitioner()

try:
    partition = partitioner.create(device=0, memory=0.9, duration=3600)
except ChronosError as e:
    print(f"Failed: {e}")

    # Try with less memory
    available = partitioner.get_available(device=0) / 100.0
    partition = partitioner.create(device=0, memory=available * 0.8, duration=3600)
```

### Time Remaining

```python
partition = partitioner.create(device=0, memory=0.5, duration=3600)

# Check time remaining
time_left = partition.time_remaining
print(f"Time remaining: {time_left} seconds")

# Save checkpoint if time is running out
if time_left < 300:  # Less than 5 minutes
    print("Time almost up, saving checkpoint...")
    torch.save(model.state_dict(), "checkpoint.pt")
```

---

## Use Cases

### Case 1: Individual Research

**Scenario:** You're a grad student training models on a shared workstation.

```python
from chronos import Partitioner
import torch

partitioner = Partitioner()

# Request 40% for 4 hours
with partitioner.create(device=0, memory=0.4, duration=14400) as p:
    print(f"Training with {p.memory_fraction*100}% GPU memory")

    model = torch.nn.Sequential(
        torch.nn.Linear(784, 256),
        torch.nn.ReLU(),
        torch.nn.Linear(256, 10)
    ).cuda()

    for epoch in range(100):
        # Training loop
        if p.time_remaining < 600:  # 10 minutes left
            torch.save(model.state_dict(), f"checkpoint_epoch_{epoch}.pt")
            break
```

### Case 2: Team GPU Allocation

**Scenario:** Lab has one GPU, five team members need fair access.

**Admin setup:**
```bash
#!/bin/bash
# setup_team.sh - Run daily at 9am

# Alice: ML training (30%, 8 hours)
sudo chronos create 0 0.30 28800 --user alice

# Bob: Inference testing (20%, 8 hours)
sudo chronos create 0 0.20 28800 --user bob

# Carol: Experimentation (15%, 8 hours)
sudo chronos create 0 0.15 28800 --user carol

# Dave: Development (15%, 8 hours)
sudo chronos create 0 0.15 28800 --user dave

# Keep 20% free for ad-hoc use

echo "Team allocations set for today:"
chronos list
```

**Team member usage:**
```bash
# Alice can now use her allocation
chronos list  # See her partition
python train_model.py  # Uses her 30%
```

### Case 3: Classroom Setup

**Scenario:** Teaching ML course, 20 students, 2 GPUs.

**Instructor setup:**
```python
from chronos import Partitioner

partitioner = Partitioner()

students = ["student1", "student2", ..., "student20"]
gpu = 0
memory_per_student = 0.05  # 5% each
duration = 3600  # 1 hour lab session

for i, student in enumerate(students):
    # Alternate between GPUs
    device = i % 2

    try:
        partition = partitioner.create(
            device=device,
            memory=memory_per_student,
            duration=duration,
            user=student
        )
        print(f"Allocated GPU {device} to {student}: {partition.partition_id}")
    except ChronosError as e:
        print(f"Failed to allocate for {student}: {e}")
```

### Case 4: CI/CD Pipeline

**Scenario:** Automated testing of GPU code.

```bash
#!/bin/bash
# test_pipeline.sh

# Request GPU for test duration
chronos create 0 0.3 300  # 5 minutes

# Get partition ID
PARTITION_ID=$(chronos list | grep "ID:" | awk '{print $2}' | head -1)

# Run tests
pytest tests/gpu/ --gpu-memory 0.3

# Cleanup
chronos release $PARTITION_ID

echo "Tests complete"
```

### Case 5: Jupyter Notebook

**Scenario:** Interactive data science work.

```python
# In Jupyter notebook
from chronos import Partitioner

# Create partition for session
partitioner = Partitioner()
partition = partitioner.create(device=0, memory=0.5, duration=7200)  # 2 hours

print(f"Partition: {partition.partition_id}")
print(f"Time remaining: {partition.time_remaining} seconds")

# Now safe to use GPU
import tensorflow as tf
model = tf.keras.Sequential([...])
model.fit(X_train, y_train)

# Check time periodically
print(f"Time remaining: {partition.time_remaining} seconds")

# Release when done
partition.release()
```

---

## Best Practices

### 1. Request Only What You Need

```bash
# Bad: Request too much
chronos create 0 0.9 86400  # 90% for 24 hours

# Good: Request reasonable amount
chronos create 0 0.5 14400  # 50% for 4 hours
```

### 2. Release Early When Possible

```python
with partitioner.create(device=0, memory=0.5, duration=3600) as p:
    # Your work
    train_model()
    # Automatic release when done, not after full hour
```

### 3. Check Availability First

```bash
# Check before requesting
available=$(chronos available 0)
echo "Available: $available%"

if (( $(echo "$available < 50" | bc -l) )); then
    echo "Not enough memory available, try later"
    exit 1
fi

chronos create 0 0.5 3600
```

### 4. Handle Errors Gracefully

```python
try:
    partition = partitioner.create(device=0, memory=0.7, duration=3600)
except ChronosError:
    # Fall back to less memory
    partition = partitioner.create(device=0, memory=0.3, duration=3600)
```

### 5. Save Checkpoints

```python
with partitioner.create(device=0, memory=0.5, duration=3600) as p:
    for epoch in range(1000):
        train_epoch()

        # Save every 10 epochs or when time low
        if epoch % 10 == 0 or p.time_remaining < 600:
            torch.save(model.state_dict(), f"checkpoint_{epoch}.pt")
```

### 6. Use Appropriate Durations

```python
# Short tasks: minutes
partitioner.create(device=0, memory=0.2, duration=600)  # 10 min test

# Medium tasks: hours
partitioner.create(device=0, memory=0.5, duration=7200)  # 2 hour training

# Long tasks: break into chunks
for chunk in range(4):
    with partitioner.create(device=0, memory=0.5, duration=7200) as p:
        train_chunk(chunk)  # 2 hours each = 8 hours total
```

### 7. Coordinate with Team

```bash
# Check who's using GPU before requesting
chronos list

# If someone using it, wait or ask
# If free, take what you need
chronos create 0 0.5 3600
```

### 8. Monitor Usage

```python
partition = partitioner.create(device=0, memory=0.5, duration=3600)

while training:
    # Periodically check time
    if partition.time_remaining < 300:
        print("5 minutes left!")
        save_checkpoint()
        break
```

---

## Troubleshooting

### "Not enough available memory"

**Cause:** Requested more memory than available

**Solution:**
```bash
# Check availability
chronos available 0

# Request less
chronos create 0 0.3 3600  # Instead of 0.5
```

### "GPU portion is locked by another user"

**Cause:** Overlapping memory range with existing partition

**Solution:**
```bash
# Check active partitions
chronos list

# Wait for expiration or use different fraction
chronos create 0 0.2 3600  # Different size
```

### "Permission denied: only administrators can create partitions for other users"

**Cause:** Non-admin trying to use `--user` flag

**Solution:**
```bash
# Remove --user flag
chronos create 0 0.5 3600

# Or use sudo
sudo chronos create 0 0.5 3600 --user alice
```

### Partition doesn't expire

**Cause:** Monitor thread may have crashed

**Solution:**
```bash
# List partitions
chronos list

# Manually release
chronos release partition_0001

# Or restart system if necessary
```

### Python: "Could not find Chronos library"

**Cause:** Library path not set

**Solution:**
```bash
# Set library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Or add to shell config
echo 'export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
```

---

## Advanced Topics

### System Integration

**Systemd service (Linux):**
```ini
# /etc/systemd/system/chronos-monitor.service
[Unit]
Description=Chronos GPU Monitor
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/chronos stats
Restart=always
User=root

[Install]
WantedBy=multi-user.target
```

**Cron job:**
```cron
# Clean up expired partitions daily
0 0 * * * /usr/local/bin/chronos list > /var/log/chronos_daily.log
```

### Scripting

**Batch job submission:**
```bash
#!/bin/bash
# submit_job.sh

JOB_SCRIPT=$1
GPU_MEMORY=${2:-0.5}
DURATION=${3:-3600}

PARTITION_ID=$(chronos create 0 $GPU_MEMORY $DURATION 2>&1 | grep "partition_" | awk '{print $3}')

if [ -z "$PARTITION_ID" ]; then
    echo "Failed to create partition"
    exit 1
fi

echo "Running $JOB_SCRIPT with partition $PARTITION_ID"
bash $JOB_SCRIPT

chronos release $PARTITION_ID
echo "Job complete"
```

### Monitoring

**Watch active partitions:**
```bash
watch -n 5 'chronos list'
```

**Log all operations:**
```bash
chronos list >> /var/log/chronos_operations.log
```

---

## Next Steps

- **Examples:** See [examples/](../examples/) for complete code samples
- **Python API:** Full reference in [python/README.md](../python/README.md)
- **FAQ:** Common questions in [FAQ.md](FAQ.md)
- **Contributing:** Help improve Chronos via [CONTRIBUTING.md](../CONTRIBUTING.md)

---

## Support

- **Issues:** https://github.com/oabraham1/chronos/issues
- **Discussions:** https://github.com/oabraham1/chronos/discussions
- **Email:** abrahamojima2018@gmail.com
