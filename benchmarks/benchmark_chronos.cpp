/**
 * @file benchmark_chronos.cpp
 * @brief Comprehensive benchmark suite for Chronos GPU Partitioner
 *
 * Copyright (c) 2025 Ojima Abraham
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Ojima Abraham
 * @date 2025
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

#include "chronos.h"

using namespace std::chrono;

class ChronosBenchmark {
   public:
    /**
     * @struct BenchmarkResult
     * @brief Stores the results of a single benchmark test.
     */
    struct BenchmarkResult {
        std::string name;
        double mean_ms;
        double stddev_ms;
        double min_ms;
        double max_ms;
        int iterations;
    };

    /**
     * @brief Benchmark partition creation latency.
     * @param iterations The number of times to run the test.
     * @return A BenchmarkResult struct with the statistics.
     */
    BenchmarkResult benchmarkPartitionCreation(int iterations = 100) {
        chronos::ChronosPartitioner partitioner;
        std::vector<double> latencies;

        for (int i = 0; i < iterations; i++) {
            auto start = high_resolution_clock::now();
            std::string partitionId = partitioner.createPartition(0, 0.1f, 5);
            auto end = high_resolution_clock::now();

            if (!partitionId.empty()) {
                duration<double, std::milli> latency = end - start;
                latencies.push_back(latency.count());
                partitioner.releasePartition(partitionId);
            }

            // Brief delay to allow system to settle
            std::this_thread::sleep_for(milliseconds(10));
        }

        return calculateStats("Partition Creation", latencies);
    }

    /**
     * @brief Benchmark partition release latency.
     * @param iterations The number of times to run the test.
     * @return A BenchmarkResult struct with the statistics.
     */
    BenchmarkResult benchmarkPartitionRelease(int iterations = 100) {
        chronos::ChronosPartitioner partitioner;
        std::vector<double> latencies;

        for (int i = 0; i < iterations; i++) {
            std::string partitionId = partitioner.createPartition(0, 0.1f, 10);

            if (!partitionId.empty()) {
                auto start = high_resolution_clock::now();
                partitioner.releasePartition(partitionId);
                auto end = high_resolution_clock::now();
                duration<double, std::milli> latency = end - start;
                latencies.push_back(latency.count());
            }

            std::this_thread::sleep_for(milliseconds(10));
        }

        return calculateStats("Partition Release", latencies);
    }

    /**
     * @brief Benchmark scalability with an increasing number of concurrent
     * partitions.
     * @param maxPartitions The maximum number of concurrent partitions to test.
     * @return A BenchmarkResult struct with the statistics on average creation
     * time.
     */
    BenchmarkResult benchmarkScalability(int maxPartitions = 10) {
        chronos::ChronosPartitioner partitioner;
        std::vector<double> avg_latencies;

        for (int numPartitions = 1; numPartitions <= maxPartitions; numPartitions++) {
            std::vector<std::string> partitionIds;
            auto start = high_resolution_clock::now();

            for (int i = 0; i < numPartitions; i++) {
                std::string id = partitioner.createPartition(0, 0.05f, 30);
                if (!id.empty()) {
                    partitionIds.push_back(id);
                }
            }

            auto end = high_resolution_clock::now();
            if (!partitionIds.empty()) {
                duration<double, std::milli> total_latency = end - start;
                avg_latencies.push_back(total_latency.count() / partitionIds.size());
            }

            for (const auto& id : partitionIds) {
                partitioner.releasePartition(id);
            }
            std::this_thread::sleep_for(milliseconds(100));
        }

        return calculateStats("Scalability Test (Avg Creation)", avg_latencies);
    }

    /**
     * @brief Benchmark the accuracy of partition expiration timing.
     * @param iterations The number of times to run the test.
     * @return A BenchmarkResult struct with statistics on the timing error in ms.
     */
    BenchmarkResult benchmarkExpirationAccuracy(int iterations = 10) {
        chronos::ChronosPartitioner partitioner;
        std::vector<double> timing_errors;

        for (int i = 0; i < iterations; i++) {
            int expected_duration_s = 2;

            auto start = high_resolution_clock::now();
            std::string partitionId = partitioner.createPartition(0, 0.1f, expected_duration_s);

            if (!partitionId.empty()) {
                // Poll until the partition expires
                while (true) {
                    auto partitions = partitioner.listPartitions(false);
                    bool found = false;
                    for (const auto& p : partitions) {
                        if (p.partitionId == partitionId) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) break;
                    std::this_thread::sleep_for(milliseconds(50));
                }

                auto end = high_resolution_clock::now();
                duration<double> actual_duration = end - start;
                double error_ms = (actual_duration.count() - expected_duration_s) * 1000.0;
                timing_errors.push_back(std::abs(error_ms));
            }
        }

        return calculateStats("Expiration Accuracy (Error)", timing_errors);
    }

    /**
     * @brief Run all benchmarks and generate a report.
     * @param outputFile The path to the CSV file for saving results.
     */
    void runAllBenchmarks(const std::string& outputFile = "chronos_benchmark_results.csv") {
        std::cout << "=== Chronos GPU Partitioner Benchmark Suite ===" << std::endl;
        std::cout << "Running comprehensive performance evaluation..." << std::endl << std::endl;

        std::vector<BenchmarkResult> results;

        std::cout << "1. Testing partition creation latency..." << std::endl;
        results.push_back(benchmarkPartitionCreation());

        std::cout << "2. Testing partition release latency..." << std::endl;
        results.push_back(benchmarkPartitionRelease());

        std::cout << "3. Testing scalability..." << std::endl;
        results.push_back(benchmarkScalability());

        std::cout << "4. Testing expiration accuracy..." << std::endl;
        results.push_back(benchmarkExpirationAccuracy());

        printResults(results);
        saveResultsToCSV(results, outputFile);
    }

   private:
    /**
     * @brief Calculate statistics for a set of measurements.
     * @param name The name of the benchmark.
     * @param values A vector of measurements.
     * @return A BenchmarkResult struct.
     */
    BenchmarkResult calculateStats(const std::string& name, const std::vector<double>& values) {
        BenchmarkResult result;
        result.name = name;
        result.iterations = values.size();

        if (values.empty()) {
            result.mean_ms = result.stddev_ms = result.min_ms = result.max_ms = 0;
            return result;
        }

        double sum = std::accumulate(values.begin(), values.end(), 0.0);
        result.mean_ms = sum / values.size();

        double sq_sum = std::inner_product(values.begin(), values.end(), values.begin(), 0.0);
        result.stddev_ms = std::sqrt(sq_sum / values.size() - result.mean_ms * result.mean_ms);

        result.min_ms = *std::min_element(values.begin(), values.end());
        result.max_ms = *std::max_element(values.begin(), values.end());

        return result;
    }

    /**
     * @brief Print the benchmark results to the console.
     * @param results A vector of BenchmarkResult structs.
     */
    void printResults(const std::vector<BenchmarkResult>& results) {
        std::cout << std::endl << "=== Benchmark Results ===" << std::endl;
        std::cout << std::left << std::setw(30) << "Test Name" << std::right << std::setw(15)
                  << "Mean (ms)" << std::setw(15) << "StdDev (ms)" << std::setw(15) << "Min (ms)"
                  << std::setw(15) << "Max (ms)" << std::setw(10) << "Samples" << std::endl;
        std::cout << std::string(100, '-') << std::endl;

        for (const auto& r : results) {
            std::cout << std::left << std::setw(30) << r.name << std::right << std::fixed
                      << std::setprecision(3) << std::setw(15) << r.mean_ms << std::setw(15)
                      << r.stddev_ms << std::setw(15) << r.min_ms << std::setw(15) << r.max_ms
                      << std::setw(10) << r.iterations << std::endl;
        }
    }

    /**
     * @brief Save the benchmark results to a CSV file.
     * @param results A vector of BenchmarkResult structs.
     * @param filename The name of the output file.
     */
    void saveResultsToCSV(const std::vector<BenchmarkResult>& results,
                          const std::string& filename) {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Failed to open output file: " << filename << std::endl;
            return;
        }

        file << "Test Name,Mean (ms),StdDev (ms),Min (ms),Max (ms),Samples" << std::endl;

        for (const auto& r : results) {
            file << r.name << "," << r.mean_ms << "," << r.stddev_ms << "," << r.min_ms << ","
                 << r.max_ms << "," << r.iterations << std::endl;
        }

        file.close();
        std::cout << std::endl << "Results saved to: " << filename << std::endl;
    }
};

int main() {
    ChronosBenchmark benchmark;
    benchmark.runAllBenchmarks();
    return 0;
}
