# CUDA GPU Partitioner

[![CI](https://github.com/yourusername/cuda-gpu-partitioner/actions/workflows/ci.yml/badge.svg)](https://github.com/yourusername/cuda-gpu-partitioner/actions)
[![Docker Image](https://img.shields.io/badge/docker%20image-available-blue)](https://github.com/yourusername/cuda-gpu-partitioner/pkgs/container/cuda-gpu-partitioner)

A CUDA-based solution for managing GPU resources with temporal partitioning.

## Features
- Percentage-based GPU memory allocation
- Time-bound computation sessions
- Automatic memory reclamation
- Cloud-ready deployment

## Prerequisites
- NVIDIA GPU with Compute Capability 3.0+
- Docker (for containerized execution)
- GitHub Account (for cloud workflows)

## Local Development (via Docker)
```bash
# Build container
docker build -t gpu-partitioner .

# Run with 30% GPU allocation for 60 seconds
docker run --gpus all gpu-partitioner -p 30 -t 60

# Open interactive shell
docker run -it --gpus all gpu-partitioner bash