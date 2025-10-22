/**
 * @file chronos_c.cpp
 * @brief Description needed
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

#include "chronos_c.h"

#include <cstring>
#include <mutex>
#include <string>

#include "chronos.h"

static std::string g_last_error;
static std::mutex g_error_mutex;

static void set_last_error(const std::string& error) {
    std::lock_guard<std::mutex> lock(g_error_mutex);
    g_last_error = error;
}

extern "C" {

ChronosPartitionerHandle chronos_partitioner_create(void) {
    try {
        chronos::ChronosPartitioner* partitioner = new chronos::ChronosPartitioner();
        return static_cast<ChronosPartitionerHandle>(partitioner);
    } catch (const std::exception& e) {
        set_last_error(std::string("Failed to create partitioner: ") + e.what());
        return nullptr;
    }
}

void chronos_partitioner_destroy(ChronosPartitionerHandle handle) {
    if (handle) {
        chronos::ChronosPartitioner* partitioner =
            static_cast<chronos::ChronosPartitioner*>(handle);
        delete partitioner;
    }
}

int chronos_create_partition(ChronosPartitionerHandle handle, int device_idx, float memory_fraction,
                             int duration_seconds, const char* target_user, char* partition_id_out,
                             size_t partition_id_size) {
    if (!handle) {
        set_last_error("Invalid partitioner handle");
        return -1;
    }

    try {
        chronos::ChronosPartitioner* partitioner =
            static_cast<chronos::ChronosPartitioner*>(handle);

        std::string user = target_user ? target_user : "";
        std::string partition_id =
            partitioner->createPartition(device_idx, memory_fraction, duration_seconds, user);

        if (partition_id.empty()) {
            set_last_error("Failed to create partition");
            return -1;
        }

        if (partition_id_out && partition_id_size > 0) {
            strncpy(partition_id_out, partition_id.c_str(), partition_id_size - 1);
            partition_id_out[partition_id_size - 1] = '\0';
        }

        return 0;
    } catch (const std::exception& e) {
        set_last_error(std::string("Exception in create_partition: ") + e.what());
        return -1;
    }
}

int chronos_release_partition(ChronosPartitionerHandle handle, const char* partition_id) {
    if (!handle || !partition_id) {
        set_last_error("Invalid handle or partition_id");
        return -1;
    }

    try {
        chronos::ChronosPartitioner* partitioner =
            static_cast<chronos::ChronosPartitioner*>(handle);

        bool success = partitioner->releasePartition(partition_id);
        if (!success) {
            set_last_error("Failed to release partition");
            return -1;
        }

        return 0;
    } catch (const std::exception& e) {
        set_last_error(std::string("Exception in release_partition: ") + e.what());
        return -1;
    }
}

int chronos_list_partitions(ChronosPartitionerHandle handle, ChronosPartitionInfo* partitions_out,
                            size_t* count_inout) {
    if (!handle || !count_inout) {
        set_last_error("Invalid handle or count pointer");
        return -1;
    }

    try {
        chronos::ChronosPartitioner* partitioner =
            static_cast<chronos::ChronosPartitioner*>(handle);

        auto partitions = partitioner->listPartitions(false);

        size_t num_to_copy = partitions.size();
        if (partitions_out) {
            num_to_copy = std::min(num_to_copy, *count_inout);

            for (size_t i = 0; i < num_to_copy; i++) {
                const auto& p = partitions[i];
                ChronosPartitionInfo* info = &partitions_out[i];

                strncpy(info->partition_id, p.partitionId.c_str(), sizeof(info->partition_id) - 1);
                info->partition_id[sizeof(info->partition_id) - 1] = '\0';

                strncpy(info->username, p.username.c_str(), sizeof(info->username) - 1);
                info->username[sizeof(info->username) - 1] = '\0';

                info->device_index = 0;
                info->memory_fraction = p.memoryFraction;
                info->duration_seconds = static_cast<int>(p.duration.count());
                info->time_remaining_seconds = p.getRemainingTime();
                info->process_id = p.processId;
                info->active = p.active ? 1 : 0;
            }
        }

        *count_inout = partitions.size();
        return 0;
    } catch (const std::exception& e) {
        set_last_error(std::string("Exception in list_partitions: ") + e.what());
        return -1;
    }
}

float chronos_get_available_percentage(ChronosPartitionerHandle handle, int device_idx) {
    if (!handle) {
        set_last_error("Invalid handle");
        return -1.0f;
    }

    try {
        chronos::ChronosPartitioner* partitioner =
            static_cast<chronos::ChronosPartitioner*>(handle);

        return partitioner->getGPUAvailablePercentage(device_idx);
    } catch (const std::exception& e) {
        set_last_error(std::string("Exception in get_available_percentage: ") + e.what());
        return -1.0f;
    }
}

void chronos_show_device_stats(ChronosPartitionerHandle handle) {
    if (!handle) {
        return;
    }

    try {
        chronos::ChronosPartitioner* partitioner =
            static_cast<chronos::ChronosPartitioner*>(handle);
        partitioner->showDeviceStats();
    } catch (const std::exception& e) {
        set_last_error(std::string("Exception in show_device_stats: ") + e.what());
    }
}

const char* chronos_get_last_error(void) {
    std::lock_guard<std::mutex> lock(g_error_mutex);
    return g_last_error.c_str();
}
}
