#ifndef __LOAD_KERNEL_H__
#define __LOAD_KERNEL_H__

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

bool load_kernel(cl_context context, cl_uint n_devices, const cl_device_id *device_list, const char *program_filename, const char *kernel_name, cl_kernel &out);

#endif // __LOAD_KERNEL_H__
