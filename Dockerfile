FROM ubuntu:22.04 as builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    ocl-icd-opencl-dev \
    opencl-headers \
    pkg-config \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/chronos

COPY CMakeLists.txt .
COPY include/ include/
COPY src/ src/
COPY apps/ apps/
COPY python/ python/
COPY setup.py .

RUN mkdir -p build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && make -j$(nproc) \
    && make install

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    ocl-icd-libopencl1 \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/bin/chronos_cli /usr/local/bin/chronos
COPY --from=builder /usr/local/lib/libchronos.so* /usr/local/lib/
COPY --from=builder /usr/src/chronos/python/chronos /usr/local/lib/python3.10/dist-packages/chronos/

RUN ldconfig

RUN mkdir -p /tmp/chronos_locks && chmod 777 /tmp/chronos_locks

ENV PYTHONPATH=/usr/local/lib/python3.10/dist-packages

ENTRYPOINT ["chronos"]
CMD ["help"]
