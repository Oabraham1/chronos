/**
 * @file advanced_usage.cpp
 * @brief Advanced example of using Chronos GPU Partitioner.
 *
 * This example demonstrates more advanced usage patterns, such as using
 * multiple partitions across different GPUs, handling partition failure
 * gracefully, and using OpenCL with the partitions.
 *
 * @author Ojima Abraham
 * @date 2025
 * @copyright MIT License
 */

#include "chronos.h"
#include "platform/opencl_include.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

std::mutex consoleMutex;

/**
 * @brief Simple vector addition kernel in OpenCL
 */
const char *vectorAdditionKernel = R"(
__kernel void vector_add(__global const float* a, __global const float* b, __global float* c, const int n) {
    int id = get_global_id(0);
    if (id < n) {
        c[id] = a[id] + b[id];
    }
}
)";

/**
 * @class GpuTask
 * @brief Class representing a GPU task using OpenCL
 */
class GpuTask {
public:
  /**
   * @brief Constructor
   *
   * @param deviceIdx GPU device index
   * @param partitionId Chronos partition ID
   * @param memFraction Memory fraction
   * @param vectorSize Size of vectors for computation
   */
  GpuTask(int deviceIdx, const std::string &partitionId, float memFraction,
          size_t vectorSize)
      : deviceIdx(deviceIdx), partitionId(partitionId),
        memoryFraction(memFraction), vectorSize(vectorSize), running(false) {}

  /**
   * @brief Destroy task resources
   */
  ~GpuTask() { cleanup(); }

  /**
   * @brief Initialize OpenCL resources
   *
   * @return True if successful, false otherwise
   */
  bool initialize() {
    cl_int err;

    cl_uint numPlatforms;
    err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
      printError("No OpenCL platforms found");
      return false;
    }

    std::vector<cl_platform_id> platforms(numPlatforms);
    err = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
    if (err != CL_SUCCESS) {
      printError("Failed to get OpenCL platform IDs");
      return false;
    }

    platform = platforms[0];

    cl_uint numDevices;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
    if (err != CL_SUCCESS || numDevices == 0) {
      printError("No OpenCL devices found");
      return false;
    }

    std::vector<cl_device_id> devices(numDevices);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices,
                         devices.data(), nullptr);
    if (err != CL_SUCCESS) {
      printError("Failed to get OpenCL device IDs");
      return false;
    }

    if (deviceIdx >= static_cast<int>(devices.size())) {
      printError("Device index out of range");
      return false;
    }

    device = devices[deviceIdx];

    cl_context_properties props[] = {CL_CONTEXT_PLATFORM,
                                     (cl_context_properties)platform, 0};

    /**
     * @brief Main entry point
     *
     * @param argc Argument count
     * @param argv Argument values
     * @return Exit code
     */
    int main(int argc, char *argv[]) {
      try {
        chronos::ChronosPartitioner partitioner;

        partitioner.showDeviceStats();

        auto partitions = partitioner.listPartitions(true);

        std::vector<std::unique_ptr<GpuTask>> tasks;
        std::vector<std::string> partitionIds;

        for (int deviceIdx = 0; deviceIdx < 2; deviceIdx++) {
          float memFraction = 0.3;
          int duration = 60;

          std::string partitionId =
              partitioner.createPartition(deviceIdx, memFraction, duration);
          if (!partitionId.empty()) {
            partitionIds.push_back(partitionId);

            size_t vectorSize = static_cast<size_t>(1000000 * memFraction);
            auto task = std::make_unique<GpuTask>(deviceIdx, partitionId,
                                                  memFraction, vectorSize);

            if (task->initialize()) {
              tasks.push_back(std::move(task));
            }
          }
        }

        if (tasks.empty()) {
          std::cerr << "Failed to create any GPU tasks" << std::endl;
          return 1;
        }

        partitioner.listPartitions(true);

        for (auto &task : tasks) {
          task->run(100);
        }

        for (auto &task : tasks) {
          task->wait();
        }

        for (const auto &partitionId : partitionIds) {
          partitioner.releasePartition(partitionId);
        }

        partitioner.showDeviceStats();

        return 0;
      } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
      }
    }

    context = clCreateContext(props, 1, &device, nullptr, nullptr, &err);
    if (err != CL_SUCCESS) {
      printError("Failed to create OpenCL context");
      return false;
    }

#ifdef CL_VERSION_2_0
    queue = clCreateCommandQueueWithProperties(context, device, nullptr, &err);
#else
    queue = clCreateCommandQueue(context, device, 0, &err);
#endif

    if (err != CL_SUCCESS) {
      printError("Failed to create command queue");
      return false;
    }

    program = clCreateProgramWithSource(context, 1, &vectorAdditionKernel,
                                        nullptr, &err);
    if (err != CL_SUCCESS) {
      printError("Failed to create program");
      return false;
    }

    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
      size_t logSize;
      clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr,
                            &logSize);
      std::vector<char> buildLog(logSize);
      clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize,
                            buildLog.data(), nullptr);

      printError("Failed to build program: " + std::string(buildLog.data()));
      return false;
    }

    kernel = clCreateKernel(program, "vector_add", &err);
    if (err != CL_SUCCESS) {
      printError("Failed to create kernel");
      return false;
    }

    h_a.resize(vectorSize);
    h_b.resize(vectorSize);
    h_c.resize(vectorSize);

    for (size_t i = 0; i < vectorSize; i++) {
      h_a[i] = static_cast<float>(i);
      h_b[i] = static_cast<float>(i * 2);
    }

    d_a = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         sizeof(float) * vectorSize, h_a.data(), &err);
    if (err != CL_SUCCESS) {
      printError("Failed to create buffer d_a");
      return false;
    }

    d_b = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         sizeof(float) * vectorSize, h_b.data(), &err);
    if (err != CL_SUCCESS) {
      printError("Failed to create buffer d_b");
      return false;
    }

    d_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * vectorSize,
                         nullptr, &err);
    if (err != CL_SUCCESS) {
      printError("Failed to create buffer d_c");
      return false;
    }

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_b);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_c);
    err |= clSetKernelArg(kernel, 3, sizeof(int), &vectorSize);

    if (err != CL_SUCCESS) {
      printError("Failed to set kernel arguments");
      return false;
    }

    printMessage("Task initialized successfully");
    return true;
  }

  /**
   * @brief Run the GPU task
   *
   * @param iterations Number of iterations to run
   */
  void run(int iterations) {
    if (!running.exchange(true)) {
      thread = std::thread(&GpuTask::runTask, this, iterations);
    }
  }

  /**
   * @brief Wait for task completion
   */
  void wait() {
    if (thread.joinable()) {
      thread.join();
    }
  }

private:
  /**
   * @brief Thread function to run the GPU task
   *
   * @param iterations Number of iterations to run
   */
  void runTask(int iterations) {
    printMessage("Starting task on partition " + partitionId);

    for (int iter = 0; iter < iterations && running; iter++) {
      cl_int err;

      size_t globalWorkSize = vectorSize;
      size_t localWorkSize = 64; // Adjust based on your GPU

      err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &globalWorkSize,
                                   &localWorkSize, 0, nullptr, nullptr);
      if (err != CL_SUCCESS) {
        printError("Failed to execute kernel (iteration " +
                   std::to_string(iter) + ")");
        break;
      }

      if (iter % 10 == 0 || iter == iterations - 1) {
        err = clEnqueueReadBuffer(queue, d_c, CL_TRUE, 0,
                                  sizeof(float) * vectorSize, h_c.data(), 0,
                                  nullptr, nullptr);
        if (err != CL_SUCCESS) {
          printError("Failed to read results");
          break;
        }

        bool valid = true;
        for (size_t i = 0; i < std::min<size_t>(10, vectorSize); i++) {
          float expected = h_a[i] + h_b[i];
          if (std::abs(h_c[i] - expected) > 1e-5) {
            valid = false;
            break;
          }
        }

        printMessage("Iteration " + std::to_string(iter) + ": " +
                     (valid ? "Valid" : "Invalid") + " results");
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    printMessage("Task completed on partition " + partitionId);
    running = false;
  }

  /**
   * @brief Clean up OpenCL resources
   */
  void cleanup() {
    running = false;

    if (thread.joinable()) {
      thread.join();
    }

    if (d_a)
      clReleaseMemObject(d_a);
    if (d_b)
      clReleaseMemObject(d_b);
    if (d_c)
      clReleaseMemObject(d_c);
    if (kernel)
      clReleaseKernel(kernel);
    if (program)
      clReleaseProgram(program);
    if (queue)
      clReleaseCommandQueue(queue);
    if (context)
      clReleaseContext(context);

    d_a = nullptr;
    d_b = nullptr;
    d_c = nullptr;
    kernel = nullptr;
    program = nullptr;
    queue = nullptr;
    context = nullptr;
  }

  /**
   * @brief Print an error message
   *
   * @param message Error message
   */
  void printError(const std::string &message) {
    std::lock_guard<std::mutex> lock(consoleMutex);
    std::cerr << "[GPU " << deviceIdx << ", Partition " << partitionId
              << "] ERROR: " << message << std::endl;
  }

  /**
   * @brief Print an informational message
   *
   * @param message Informational message
   */
  void printMessage(const std::string &message) {
    std::lock_guard<std::mutex> lock(consoleMutex);
    std::cout << "[GPU " << deviceIdx << ", Partition " << partitionId << "] "
              << message << std::endl;
  }

  int deviceIdx;
  std::string partitionId;
  float memoryFraction;
  size_t vectorSize;

  cl_platform_id platform = nullptr;
  cl_device_id device = nullptr;
  cl_context context = nullptr;
  cl_command_queue queue = nullptr;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_mem d_a = nullptr;
  cl_mem d_b = nullptr;
  cl_mem d_c = nullptr;

  std::vector<float> h_a;
  std::vector<float> h_b;
  std::vector<float> h_c;

  std::atomic<bool> running;
  std::thread thread;
};