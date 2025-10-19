/**
 * @file chronos_c.h
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

#ifndef CHRONOS_C_H
#define CHRONOS_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef void* ChronosPartitionerHandle;
typedef void* ChronosPartitionHandle;

typedef struct {
    char partition_id[64];
    int device_index;
    float memory_fraction;
    int duration_seconds;
    int time_remaining_seconds;
    char username[256];
    int process_id;
    int active;
} ChronosPartitionInfo;

ChronosPartitionerHandle chronos_partitioner_create(void);

void chronos_partitioner_destroy(ChronosPartitionerHandle handle);

int chronos_create_partition(ChronosPartitionerHandle handle, int device_idx, float memory_fraction,
                             int duration_seconds, const char* target_user, char* partition_id_out,
                             size_t partition_id_size);

int chronos_release_partition(ChronosPartitionerHandle handle, const char* partition_id);

int chronos_list_partitions(ChronosPartitionerHandle handle, ChronosPartitionInfo* partitions_out,
                            size_t* count_inout);

float chronos_get_available_percentage(ChronosPartitionerHandle handle, int device_idx);

void chronos_show_device_stats(ChronosPartitionerHandle handle);

const char* chronos_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
