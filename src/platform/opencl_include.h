/**
 * @file opencl_include.h
 * @brief Platform-specific OpenCL include with fallback for CI environments
 */

#ifndef CHRONOS_OPENCL_INCLUDE_H
#define CHRONOS_OPENCL_INCLUDE_H

#ifdef SKIP_OPENCL_TESTS
// Mock OpenCL types and defines for CI environments without OpenCL
typedef void *cl_platform_id;
typedef void *cl_device_id;
typedef void *cl_context;
typedef void *cl_command_queue;
typedef void *cl_program;
typedef void *cl_kernel;
typedef void *cl_mem;
typedef int cl_int;
typedef unsigned int cl_uint;
typedef unsigned long cl_ulong;
typedef unsigned int cl_device_type;
typedef long cl_context_properties; // Add this for the props array

#define CL_SUCCESS 0
#define CL_DEVICE_TYPE_ALL 0
#define CL_DEVICE_TYPE_CPU 1
#define CL_DEVICE_TYPE_GPU 2
#define CL_DEVICE_TYPE_ACCELERATOR 4
#define CL_DEVICE_TYPE_DEFAULT 8
#define CL_DEVICE_NAME 0x102B
#define CL_DEVICE_TYPE 0x1000
#define CL_DEVICE_GLOBAL_MEM_SIZE 0x101F
#define CL_DEVICE_VENDOR 0x102C
#define CL_DEVICE_VERSION 0x102F
#define CL_CONTEXT_PLATFORM 0x1084 // Add this for the props array

// Mock OpenCL functions for CI environments
inline cl_int clGetPlatformIDs(cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms)
{
    if (num_platforms)
        *num_platforms = 0;
    return CL_SUCCESS;
}

inline cl_int clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type,
                             cl_uint num_entries, cl_device_id *devices, cl_uint *num_devices)
{
    if (num_devices)
        *num_devices = 0;
    return CL_SUCCESS;
}

inline cl_int clGetDeviceInfo(cl_device_id device, cl_uint param_name,
                              size_t param_value_size, void *param_value, size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

inline cl_context clCreateContext(const cl_context_properties *properties, cl_uint num_devices,
                                  const cl_device_id *devices, void *pfn_notify,
                                  void *user_data, cl_int *errcode_ret)
{
    if (errcode_ret)
        *errcode_ret = CL_SUCCESS;
    return nullptr;
}

inline cl_int clReleaseContext(cl_context context)
{
    return CL_SUCCESS;
}

inline cl_int clReleaseMemObject(cl_mem memobj)
{
    return CL_SUCCESS;
}

inline cl_int clReleaseKernel(cl_kernel kernel)
{
    return CL_SUCCESS;
}

inline cl_int clReleaseProgram(cl_program program)
{
    return CL_SUCCESS;
}

inline cl_int clReleaseCommandQueue(cl_command_queue command_queue)
{
    return CL_SUCCESS;
}
#else
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#endif

#endif // CHRONOS_OPENCL_INCLUDE_H