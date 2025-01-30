#include "include/gpu_partitioner.h"
#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <future>
#include <cuda_runtime.h>

class GPUPartitionerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Verify CUDA is available
        cudaError_t err = cudaDeviceReset();
        ASSERT_EQ(err, cudaSuccess) << "CUDA device initialization failed";
    }

    void TearDown() override {
        cudaDeviceReset();
    }

    static constexpr float TEST_DURATION = 1.0f; // Seconds
};

TEST_F(GPUPartitionerTest, BasicSessionCreation) {
    GPUPartitioner partitioner;

    EXPECT_TRUE(partitioner.createSession(10.0f, TEST_DURATION));
    EXPECT_NEAR(partitioner.getUsedMemory() / (float)partitioner.getTotalMemory(),
                0.1f, 0.01f);
}

TEST_F(GPUPartitionerTest, MemoryReclamation) {
    GPUPartitioner partitioner;

    ASSERT_TRUE(partitioner.createSession(15.0f, 0.5f));
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    partitioner.cleanupFinishedSessions();
    EXPECT_EQ(partitioner.getUsedMemory(), 0);
}

TEST_F(GPUPartitionerTest, ConcurrentSessions) {
    GPUPartitioner partitioner;
    constexpr int NUM_THREADS = 4;
    std::vector<std::future<bool>> futures;

    auto create_session = [&](float percentage) {
        return partitioner.createSession(percentage, TEST_DURATION);
    };

    // Create concurrent sessions
    futures.push_back(std::async(std::launch::async, create_session, 10.0f));
    futures.push_back(std::async(std::launch::async, create_session, 15.0f));
    futures.push_back(std::async(std::launch::async, create_session, 20.0f));

    // Validate results
    int success_count = 0;
    for(auto& future : futures) {
        if(future.get()) success_count++;
    }

    EXPECT_EQ(success_count, 3);
    EXPECT_NEAR(partitioner.getUsedMemory() / (float)partitioner.getTotalMemory(),
                0.45f, 0.01f);
}

TEST_F(GPUPartitionerTest, ComputationValidation) {
    GPUPartitioner partitioner;
    ASSERT_TRUE(partitioner.createSession(10.0f, TEST_DURATION));

    // Get device pointer from session (requires friend access)
    auto& session = *partitioner.sessions[0];
    float* d_data = reinterpret_cast<float*>(session.d_memory);
    const size_t num_elements = session.mem_size / sizeof(float);

    // Initialize host memory
    std::vector<float> host_data(num_elements, 0.0f);

    // Copy to device and run computation
    cudaMemcpy(d_data, host_data.data(), session.mem_size, cudaMemcpyHostToDevice);
    session.runComputation();
    cudaMemcpy(host_data.data(), d_data, session.mem_size, cudaMemcpyDeviceToHost);

    // Verify computation results
    bool non_zero_found = false;
    for(auto val : host_data) {
        if(val != 0.0f) {
            non_zero_found = true;
            break;
        }
    }
    EXPECT_TRUE(non_zero_found);
}

TEST_F(GPUPartitionerTest, EdgeCaseHandling) {
    GPUPartitioner partitioner;

    // Invalid percentages
    EXPECT_FALSE(partitioner.createSession(-5.0f, TEST_DURATION));
    EXPECT_FALSE(partitioner.createSession(101.0f, TEST_DURATION));

    // Valid edge cases
    EXPECT_TRUE(partitioner.createSession(0.1f, TEST_DURATION));
    EXPECT_TRUE(partitioner.createSession(95.0f, TEST_DURATION));

    // Over-allocation
    EXPECT_FALSE(partitioner.createSession(10.0f, TEST_DURATION));
}

TEST_F(GPUPartitionerTest, ThreadSafety) {
    GPUPartitioner partitioner;
    constexpr int NUM_THREADS = 8;
    std::atomic<int> success_count{0};

    auto worker = [&](float percentage) {
        if(partitioner.createSession(percentage, TEST_DURATION)) {
            success_count++;
        }
        partitioner.cleanupFinishedSessions();
    };

    std::vector<std::thread> threads;
    for(int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, 10.0f + i);
    }

    for(auto& t : threads) {
        t.join();
    }

    // Validate memory consistency
    const size_t total_allocated = partitioner.getUsedMemory();
    const size_t total_memory = partitioner.getTotalMemory();
    EXPECT_LE(total_allocated, total_memory);
    EXPECT_GE(success_count, 3); // At least some should succeed
}

TEST_F(GPUPartitionerTest, ResourceCleanup) {
    {
        GPUPartitioner partitioner;
        ASSERT_TRUE(partitioner.createSession(20.0f, TEST_DURATION));
    } // Partitioner destroyed here

    // Verify GPU memory is freed
    size_t free_after, total_after;
    cudaMemGetInfo(&free_after, &total_after);

    GPUPartitioner new_partitioner;
    EXPECT_TRUE(new_partitioner.createSession(95.0f, TEST_DURATION));
}

// Death tests for fatal error conditions
TEST_F(GPUPartitionerTest, InvalidInitialization) {
    // Simulate CUDA init failure
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_DEATH({
        GPUPartitioner partitioner;
        throw std::runtime_error("Force CUDA init failure");
    }, ".*CUDA context initialization failed.*");
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
