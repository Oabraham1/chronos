FROM nvcr.io/nvidia/cuda:12.2.0-devel-ubuntu22.04

# Install samples via package manager instead of script
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libgtest-dev \
    cuda-samples-12-2 \
    && rm -rf /var/lib/apt/lists/*

# Copy samples to workspace
RUN cp -r /usr/local/cuda/samples /workspace/cuda_samples

# Verify samples compilation
RUN cd /workspace/cuda_samples/0_Simple/vectorAdd && make

# Rest of your configuration...
WORKDIR /workspace
