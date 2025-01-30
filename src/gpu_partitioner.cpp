#include "gpu_partitioner.h"
#include <cuda_runtime.h>
#include <stdexcept>
#include <string>

GPUPartitioner::GPUPartitioner(bool enable_safety_checks) 
    : safety_checks_(enable_safety_checks) {
    
    size_t free, total;
    cudaError_t err = cudaMemGetInfo(&free, &total);
    
    if(err != cudaSuccess) {
        throw std::runtime_error("Failed to initialize GPU context: " + 
                               std::string(cudaGetErrorString(err)));
    }
    
    if(total == 0) {
        throw std::runtime_error("No available GPU memory");
    }
    
    total_mem_ = total;
}

GPUPartitioner::~GPUPartitioner() {
    std::lock_guard<std::mutex> lock(partition_mutex_);
    sessions_.clear();
    used_mem_.store(0, std::memory_order_relaxed);
}

bool GPUPartitioner::create_session(float percentage, float duration, 
                                   GPUSession::ComputationCallback cb) {
    if(percentage < 0.1f || percentage > 95.0f) {
        return false;
    }

    const size_t requested = static_cast<size_t>((percentage / 100.0f) * total_mem_);
    
    try {
        std::lock_guard<std::mutex> lock(partition_mutex_);
        
        if(safety_checks_) {
            validate_memory_request(requested);
        }

        sessions_.emplace_back(
            std::make_unique<GPUSession>(requested, duration, std::move(cb))
        );
        used_mem_ += requested;
        return true;
    } 
    catch(const std::bad_alloc& e) {
        return false;
    }
    catch(const std::runtime_error& e) {
        return false;
    }
}

void GPUPartitioner::garbage_collect() noexcept {
    std::lock_guard<std::mutex> lock(partition_mutex_);
    
    auto it = sessions_.begin();
    while(it != sessions_.end()) {
        if(!(*it)->is_active()) {
            used_mem_ -= (*it)->allocated_size();
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

void GPUPartitioner::execute_all() {
    std::lock_guard<std::mutex> lock(partition_mutex_);
    for(auto& session : sessions_) {
        if(session->is_active()) {
            session->execute();
        }
    }
}

size_t GPUPartitioner::used_memory() const noexcept {
    return used_mem_.load(std::memory_order_relaxed);
}

size_t GPUPartitioner::total_memory() const noexcept {
    return total_mem_;
}

float GPUPartitioner::utilization() const noexcept {
    return static_cast<float>(used_mem_) / total_mem_;
}

void GPUPartitioner::validate_memory_request(size_t requested) {
    const size_t available = calculate_available();
    
    if(requested == 0) {
        throw std::invalid_argument("Memory request must be greater than zero");
    }
    
    if(requested > available) {
        throw std::runtime_error(
            "Memory allocation error: Requested " + 
            std::to_string(requested) + " bytes, " +
            std::to_string(available) + " bytes available");
    }
}

size_t GPUPartitioner::calculate_available() const noexcept {
    size_t current_used = used_mem_.load(std::memory_order_relaxed);
    size_t safety_buffer = static_cast<size_t>(total_mem_ * 0.05f);
    return total_mem_ - current_used - safety_buffer;
}