#include "gpu_partitioner.h"
#include <chrono>
#include <cmath>

__global__ void default_computation_kernel(float* data, size_t size) {
    const size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if(idx < size) {
        float result = 0.0f;
        for(int i = 0; i < 1000; ++i) {
            const float x = data[idx] + i * 0.01f;
            result += powf(x, 3.14f) / log2f(x + 1e-6f);
        }
        data[idx] = result;
    }
}

GPUSession::GPUSession(size_t size, float duration, ComputationCallback cb)
    : capacity_(size), user_callback_(std::move(cb)) {
    
    check_cuda(cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
              "Stream creation");
    
    check_cuda(cudaMallocAsync(&d_memory_, capacity_, stream_),
              "Memory allocation");
    
    check_cuda(cudaMemsetAsync(d_memory_, 0, capacity_, stream_),
              "Memory initialization");

    running_ = true;
    monitor_thread_ = std::thread(&GPUSession::monitor_task, this, duration);
}

void GPUSession::execute() {
    std::lock_guard<std::mutex> lock(resource_mutex_);
    if(!running_ || !d_memory_) return;

    if(user_callback_) {
        user_callback_(d_memory_, capacity_/sizeof(float), stream_);
    } else {
        constexpr int block_size = 256;
        const int num_blocks = (capacity_/sizeof(float) + block_size - 1) / block_size;
        default_computation_kernel<<<num_blocks, block_size, 0, stream_>>>(
            d_memory_, capacity_/sizeof(float)
        );
    }
    check_cuda(cudaGetLastError(), "Kernel launch");
    check_cuda(cudaStreamSynchronize(stream_), "Stream sync");
}

void GPUSession::monitor_task(float duration) {
    using namespace std::chrono;
    const auto start = steady_clock::now();
    
    while(running_) {
        std::this_thread::sleep_for(100ms);
        
        if(duration_cast<seconds>(steady_clock::now() - start).count() >= duration) {
            std::lock_guard<std::mutex> lock(resource_mutex_);
            running_ = false;
            safe_cleanup();
            break;
        }
    }
}

void GPUSession::safe_cleanup() noexcept {
    if(d_memory_) {
        cudaFreeAsync(d_memory_, stream_);
        d_memory_ = nullptr;
    }
    if(stream_) {
        cudaStreamDestroy(stream_);
        stream_ = nullptr;
    }
}

GPUSession::~GPUSession() noexcept {
    running_ = false;
    if(monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    safe_cleanup();
}

void GPUSession::check_cuda(cudaError_t err, const char* context) {
    if(err != cudaSuccess) {
        throw std::runtime_error(
            std::string(context) + " failed: " + cudaGetErrorString(err)
        );
    }
}