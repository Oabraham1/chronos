#include "gpu_partitioner.h"
#include <iostream>
#include <csignal>
#include <iomanip>
#include <chrono>
#include <thread>

std::atomic<bool> exit_flag{false};

void signal_handler(int) {
    std::cout << "\nTerminating gracefully...\n";
    exit_flag = true;
}

void print_usage(const char* prog_name) {
    std::cout << "GPU Resource Partitioner\n\n"
              << "Usage: " << prog_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -p, --percent FLOAT   Memory percentage (1-95)\n"
              << "  -t, --time FLOAT      Duration in seconds\n"
              << "  -v, --verbose         Show detailed output\n"
              << "  --unsafe              Disable safety checks\n\n"
              << "Example:\n"
              << "  " << prog_name << " -p 30 -t 60 -v\n";
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);
    
    float percentage = 0;
    float duration = 0;
    bool verbose = false;
    bool safe_mode = true;

    // Parse CLI arguments
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if((arg == "-p" || arg == "--percent") && i+1 < argc) {
            percentage = std::stof(argv[++i]);
        } else if((arg == "-t" || arg == "--time") && i+1 < argc) {
            duration = std::stof(argv[++i]);
        } else if(arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if(arg == "--unsafe") {
            safe_mode = false;
        }
    }

    if(percentage <= 0 || duration <= 0) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        GPUPartitioner partitioner(safe_mode);
        
        if(!partitioner.create_session(percentage, duration, 
            [verbose](float* data, size_t size, cudaStream_t stream) {
                if(verbose) {
                    std::cout << "Executing custom kernel on " << size << " elements\n";
                }
                const int block_size = 256;
                const int num_blocks = (size + block_size - 1) / block_size;
                default_computation_kernel<<<num_blocks, block_size, 0, stream>>>(data, size);
            })) 
        {
            std::cerr << "Failed to create GPU session\n";
            return 1;
        }

        const auto start = std::chrono::steady_clock::now();
        while(!exit_flag) {
            partitioner.execute_all();
            partitioner.garbage_collect();

            if(verbose) {
                const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - start).count();
                std::cout << "\rElapsed: " << elapsed << "s | "
                          << "Used: " << std::fixed << std::setprecision(1)
                          << (partitioner.utilization() * 100) << "%"
                          << std::flush;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    } catch(const std::exception& e) {
        std::cerr << "Critical Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}