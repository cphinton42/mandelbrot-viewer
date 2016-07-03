#include <cstdio>
#include <cstdlib>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "defer.h"

int main()
{
    // get platform count
    cl_uint n_platforms;
    clGetPlatformIDs(0, nullptr, &n_platforms);

    if(n_platforms == 0)
    {
	printf("No platforms\n");
    }
    else
    {
	// get all platforms
	cl_platform_id *platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * n_platforms);
	defer { free(platforms); };
	clGetPlatformIDs(n_platforms, platforms, nullptr);

	// for each platform print all attributes
	for(int i = 0; i < n_platforms; ++i)
	{
	    printf("Platform %i\n", i+1);

	    for(int j = 0; j < 5; ++j)
	    {
		static const char* attribute_names[5] = { "Name", "Vendor", "Version", "Profile", "Extensions" };
		static const cl_platform_info attribute_types[5] = { CL_PLATFORM_NAME, CL_PLATFORM_VENDOR, CL_PLATFORM_VERSION, CL_PLATFORM_PROFILE, CL_PLATFORM_EXTENSIONS };
		
		// get platform attribute value size
		size_t info_size;
		clGetPlatformInfo(platforms[i], attribute_types[j], 0, nullptr, &info_size);
		char *info = (char*) malloc(info_size);
		defer { free(info); };

		// get platform attribute value
		clGetPlatformInfo(platforms[i], attribute_types[j], info_size, info, nullptr);

		printf("  %i.%i %-11s: %s\n", i+1, j+1, attribute_names[j], info);
	    }

	    printf("  %i.6 Devices    :\n", i+1);

	    cl_uint n_devices;
	    clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, nullptr, &n_devices);

	    cl_device_id *devices = (cl_device_id*) malloc(sizeof(cl_device_id) * n_devices);
	    defer { free(devices); };
	    clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, n_devices, devices, nullptr);

	    for(int j = 0; j < n_devices; ++j)
	    {
		printf("    Device %i\n", j+1);

		for(int k = 0; k < 2; ++k)
		{
		    static const char* attribute_names[2] = { "Name", "Version" };
		    static const cl_device_info attribute_types[2] = { CL_DEVICE_NAME, CL_DEVICE_VERSION };
		
		    size_t value_size;
		    clGetDeviceInfo(devices[j], attribute_types[k], 0, nullptr, &value_size);
		    char *value = (char*) malloc(value_size);
		    defer { free(value); };
		    clGetDeviceInfo(devices[j], attribute_types[k], value_size, value, nullptr);
		    printf("      %i.%i.%i %-14s: %s\n", i+1, j+1, k+1, attribute_names[k], value);
		}

		for(int k = 0; k < 2; ++k)
		{
		    static const char* attribute_names[2] = { "Global Mem", "Local Mem" };
		    static const cl_device_info attribute_types[2] = { CL_DEVICE_GLOBAL_MEM_SIZE, CL_DEVICE_LOCAL_MEM_SIZE };
		
		    cl_ulong value;
		    clGetDeviceInfo(devices[j], attribute_types[k], sizeof(cl_ulong), &value, nullptr);
		    printf("      %i.%i.%i %-14s: %lu\n", i+1, j+1, k+3, attribute_names[k], value);
		}

		for(int k = 0; k < 2; ++k)
		{
		    static const char* attribute_names[2] = { "Compute Units", "Clock Rate" };
		    static const cl_device_info attribute_types[2] = { CL_DEVICE_MAX_COMPUTE_UNITS, CL_DEVICE_MAX_CLOCK_FREQUENCY };
		
		    cl_uint value;
		    clGetDeviceInfo(devices[j], attribute_types[k], sizeof(cl_uint), &value, nullptr);
		    printf("      %i.%i.%i %-14s: %u\n", i+1, j+1, k+5, attribute_names[k], value);
		}

		size_t work_item_sizes[3];
		clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_ITEM_SIZES, 3*sizeof(size_t), work_item_sizes, nullptr);
		printf("      %i.%i.7 Max Work Items: %zu, %zu, %zu\n", i+1, j+1, work_item_sizes[0], work_item_sizes[1], work_item_sizes[2]);
		
	    }
	}
    }
    return 0;
}
