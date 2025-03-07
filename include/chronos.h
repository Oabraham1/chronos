/**
 * @file chronos.h
 * @brief Main header file for Chronos GPU Partitioner
 */

#ifndef CHRONOS_H
#define CHRONOS_H

#include <string>
#include <vector>
#include <chrono>
#include <memory>

// Include complete definition of core classes
#include "core/device_info.h"
#include "core/gpu_partition.h"

namespace chronos
{

    /**
     * @class ChronosPartitioner
     * @brief Main class for managing GPU partitions
     *
     * This class provides the primary interface for the Chronos GPU Partitioner.
     * It allows creation, monitoring, and management of GPU partitions.
     */
    class ChronosPartitioner
    {
    public:
        using GPUPartition = core::GPUPartition;
        using DeviceInfo = core::DeviceInfo;

        /**
         * @brief Constructor
         *
         * Initializes the partitioner, detects available devices,
         * and starts the monitoring thread.
         */
        ChronosPartitioner();

        /**
         * @brief Destructor
         *
         * Cleans up resources and releases all active partitions.
         */
        ~ChronosPartitioner();

        /**
         * @brief Create a new GPU partition
         *
         * @param deviceIdx Index of the device to use
         * @param memoryFraction Fraction of device memory to allocate (0.0-1.0)
         * @param durationInSeconds Duration for which the partition should exist
         * @return Partition ID string or empty string on failure
         */
        std::string createPartition(int deviceIdx, float memoryFraction, int durationInSeconds);

        /**
         * @brief List all active partitions
         *
         * @param printOutput Whether to print partition details to stdout
         * @return Vector of active partitions
         */
        std::vector<GPUPartition> listPartitions(bool printOutput = false);

        /**
         * @brief Release a partition early
         *
         * @param partitionId ID of the partition to release
         * @return True if successfully released, false otherwise
         */
        bool releasePartition(const std::string &partitionId);

        /**
         * @brief Display statistics for all devices
         */
        void showDeviceStats();

        /**
         * @brief Get the percentage of GPU memory available
         *
         * @param deviceIdx Index of the device to query
         * @return Percentage of memory available (0-100) or -1 on error
         */
        float getGPUAvailablePercentage(int deviceIdx);

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };

}

#endif
