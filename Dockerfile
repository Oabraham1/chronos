FROM ubuntu:22.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    ocl-icd-opencl-dev \
    opencl-headers \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Create build directory
WORKDIR /usr/src/chronos

# Copy source code
COPY . .

# Build Chronos
RUN mkdir -p build && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && make install

# Create a smaller runtime image
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    ocl-icd-libopencl1 \
    ocl-icd-opencl-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy the built executable and libraries from the builder stage
COPY --from=builder /usr/local/bin/chronos_cli /usr/local/bin/chronos
COPY --from=builder /usr/local/lib/libchronos.so* /usr/local/lib/

# Update library cache
RUN ldconfig

# Create lock directory
RUN mkdir -p /tmp/chronos_locks && chmod 777 /tmp/chronos_locks

# Default command
ENTRYPOINT ["chronos"]
CMD ["help"]
