#include <cstdio>
#include <cstring>
#include <errno.h>

#include "load_kernel.h"

bool load_kernel(cl_context context, cl_uint n_devices, const cl_device_id *device_list, const char *program_filename, const char *kernel_name, cl_kernel &out)
{
    FILE *program_file = fopen(program_filename, "r");
    if(!program_file)
    {
	fprintf(stderr, "Unable to create program: error opening file '%s': %s\n", program_filename, std::strerror(errno));
	return false;
    }
    defer { fclose(program_file); };

    fseek(program_file, 0, SEEK_END);
    long size = ftell(program_file);
    fseek(program_file, 0, SEEK_SET);

    if(ferror(program_file))
    {
	fprintf(stderr, "Unable to create program: error getting size of file '%s'\n", program_filename);
	return false;
    }

    char *buffer = (char*) malloc(size);
    defer { free(buffer); };
    
    size_t bytes_read = fread(buffer, 1, size, program_file);
    if(ferror(program_file))
    {
	fprintf(stderr, "Unable to create program: error reading from file '%s': %s\n", program_filename, std::strerror(errno));
	return false;
    }
    
    cl_int ret;
    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&buffer, (const size_t *)&size, &ret);
    if(ret == CL_OUT_OF_HOST_MEMORY)
    {
	fprintf(stderr, "Unable to create program: out of host memory\n");
	return false;
    }
    else if(ret != CL_SUCCESS)
    {
	fprintf(stderr, "Unable to create program: error creating program: error code %i\n", ret);
	return false;
    }

    defer { clReleaseProgram(program); };

    ret = clBuildProgram(program, n_devices, device_list, nullptr, nullptr, nullptr);
    if(ret == CL_BUILD_PROGRAM_FAILURE)
    {
	// TODO for all devices?
	size_t log_size;
	clGetProgramBuildInfo(program, device_list[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);

	char *program_log = (char*) malloc(log_size+1);
	defer { free(program_log); };
	program_log[log_size] = '\0';

	clGetProgramBuildInfo(program, device_list[0], CL_PROGRAM_BUILD_LOG, log_size, program_log, nullptr);
	
	fprintf(stderr, "Unable to create program: build failure\n%s\n", program_log);
	return false;
    }
    else if(ret == CL_OUT_OF_HOST_MEMORY)
    {
	fprintf(stderr, "Unable to create program: out of host memory\n");
	return false;
    }
    else if(ret != CL_SUCCESS)
    {
	fprintf(stderr, "Unable to create program: error building program: error code %i\n", ret);
	return false;
    }

/*
        clGetProgramBuildInfo(program, device, 
                CL_PROGRAM_BUILD_LOG, logSize+1, programLog, NULL);
        printf("Build failed; error=%d, status=%d, programLog:nn%s", 
                error, status, programLog);
        free(programLog);
 */
    
    out = clCreateKernel(program, kernel_name, &ret);

    if(ret == CL_INVALID_KERNEL_NAME)
    {
	fprintf(stderr, "Unable to create program: '%s' is not the name of a kernel\n", kernel_name);
	return false;
    }
    else if(ret == CL_INVALID_KERNEL_DEFINITION)
    {
	fprintf(stderr, "Unable to create program: '%s' is an invalid kernel\n", kernel_name);
	return false;
    }
    else if(ret == CL_OUT_OF_HOST_MEMORY)
    {
	fprintf(stderr, "Unable to create program: out of host memory\n");
	return false;
    }
    else if(ret != CL_SUCCESS)
    {
	fprintf(stderr, "Unable to create program: error creating kernel: error code %i\n", ret);
	return false;
    }

    return true;
}
