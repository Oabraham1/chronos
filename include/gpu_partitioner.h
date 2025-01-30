#pragma once

#include <cuda_runtime.h>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <stdexcept>

class GPUSession {
public:
    using ComputationCallback = std::function<void(float*, size_t, cudaStream_t)>;
    
    GPUSession(size_t size, float duration_sec, ComputationCallback cb = {});
    ~GPUSession() noexcept;
    
    void execute();
    bool is_active() const noexcept;
    size_t allocated_size() const noexcept;
    
    GPUSession(const GPUSession&) = delete;
    GPUSession& operator=(const GPUSession&) = delete;

private:
    float* d_memory_ = nullptr;
    cudaStream_t stream_ = nullptr;
    size_t capacity_;
    std::atomic<bool> running_{false};
    std::thread monitor_thread_;
    mutable std::mutex resource_mutex_;
    ComputationCallback user_callback_;
    
    void monitor_task(float duration);
    void safe_cleanup() noexcept;
    static void check_cuda(cudaError_t err, const char* context);
};

class GPUPartitioner {
public:
    explicit GPUPartitioner(bool enable_safety_checks = true);
    ~GPUPartitioner();
    
    bool create_session(float percentage, float duration, 
                       GPUSession::ComputationCallback cb = {});
    void garbage_collect() noexcept;
    void execute_all();
    
    [[nodiscard]] size_t used_memory() const noexcept;
    [[nodiscard]] size_t total_memory() const noexcept;
    [[nodiscard]] float utilization() const noexcept;

private:
    std::vector<std::unique_ptr<GPUSession>> sessions_;
    size_t total_mem_;
    std::atomic<size_t> used_mem_{0};
    mutable std::mutex partition_mutex_;
    bool safety_checks_;
    
    void validate_memory_request(size_t requested);
    [[nodiscard]] size_t calculate_available() const noexcept;
};