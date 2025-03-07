/**
 * @file gpu_partition.h
 * @brief GPU partition data structures
 */

#ifndef CHRONOS_GPU_PARTITION_H
#define CHRONOS_GPU_PARTITION_H

#include <string>
#include <chrono>
#include "platform/opencl_include.h"

namespace chronos
{
    namespace core
    {

        /**
         * @class GPUPartition
         * @brief Represents a GPU partition allocation
         */
        class GPUPartition
        {
        public:
            /**
             * @brief Default constructor
             */
            GPUPartition();

            /**
             * @brief Check if the partition has expired
             * @return True if expired, false otherwise
             */
            bool isExpired() const;

            /**
             * @brief Get remaining time in seconds
             * @return Seconds remaining until expiration
             */
            int getRemainingTime() const;

            cl_device_id deviceId;
            float memoryFraction;
            std::chrono::seconds duration;
            std::chrono::system_clock::time_point startTime;
            bool active;
            std::string partitionId;
            int processId;
            std::string username;
        };

    }
}

#endif
