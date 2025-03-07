# Using Chronos with Docker

This guide explains how to build and use Chronos within Docker containers.

## Prerequisites

- Docker installed on your system
- For GPU support:
  - NVIDIA Container Toolkit (for NVIDIA GPUs)
  - Or appropriate GPU passthrough configuration for AMD/Intel GPUs

## Building the Docker Image

```bash
# Build the image
docker build -t chronos:latest .

# Or using docker-compose
docker-compose build
```

## Running Chronos in Docker

### Basic Usage

```bash
# Show help
docker run --rm chronos:latest help

# Show device statistics
docker run --rm chronos:latest stats
```

### With NVIDIA GPUs

Using `docker` command:

```bash
# With NVIDIA Container Toolkit
docker run --gpus all --rm chronos:latest stats
```

Using `docker-compose`:

```bash
# Override the default command to show stats
docker-compose run chronos stats
```

### Creating Partitions

When creating partitions, the lock files need to be shared between containers and possibly with the host system. Mount the lock directory as a volume:

```bash
# Create a partition (30% of GPU 0 for 30 minutes)
docker run --gpus all --rm \
  -v /tmp/chronos_locks:/tmp/chronos_locks \
  chronos:latest create 0 0.3 1800
```

### Sharing Partitions with Host System

If you want to create partitions in Docker and use them from the host system (or vice versa), ensure the lock directory is shared:

1. Create the lock directory on the host:
   ```bash
   mkdir -p /tmp/chronos_locks
   chmod 777 /tmp/chronos_locks
   ```

2. Mount it when running Chronos in Docker:
   ```bash
   docker run --gpus all --rm \
     -v /tmp/chronos_locks:/tmp/chronos_locks \
     chronos:latest ...
   ```

## Using with docker-compose

The provided `docker-compose.yml` file simplifies running Chronos:

```bash
# Show statistics
docker-compose run chronos stats

# Create a partition
docker-compose run chronos create 0 0.5 3600

# List partitions
docker-compose run chronos list
```

## Building a Custom Chronos Application with Docker

To create a custom application that uses Chronos within Docker:

```Dockerfile
FROM chronos:latest

# Install additional dependencies
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Copy your application
COPY myapp /app
WORKDIR /app

# Your application can now use Chronos
# Example: create a partition, run your app, then release
CMD chronos create 0 0.5 3600 && \
    python3 my_application.py && \
    chronos release $(cat partition_id.txt)
```

## Troubleshooting

### No OpenCL Devices Found

Ensure GPU drivers are properly installed on the host and that GPU passthrough is correctly configured:

- For NVIDIA GPUs, make sure the NVIDIA Container Toolkit is properly installed and configured
- Check that the necessary OpenCL libraries are available in the container

### Permission Issues with Lock Files

If you encounter permission issues with lock files:

```bash
# On the host
sudo mkdir -p /tmp/chronos_locks
sudo chmod 777 /tmp/chronos_locks

# Rebuild your Docker image to ensure the permission changes are reflected
docker-compose build --no-cache
```

### Memory Allocation Errors

If you see "Not enough available memory" errors, the partition size might be too large for the available GPU memory:

- Check available memory with `docker run --gpus all --rm chronos:latest stats`
- Try a smaller memory fraction
