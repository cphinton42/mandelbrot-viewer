#include <chrono>
#include <cstdio>
#include <thread>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "glad/glad.h"
#include "glad/glad.c"

#include <GLFW/glfw3.h>

#define GLM_FORCE_CXX11 
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "typedefs.h"
#include "defer.h"

#include "load_shader.h"
#include "load_shader.cpp"
#include "load_kernel.h"
#include "load_kernel.cpp"

#define IMAGE_SIZE 2000


static float aspect_ratio = 1.0;
static float scale = 1.0;
static bool mouse_pressed = false;
static bool mouse_moved = false;
static glm::vec3 mouse_pos = {0,0,1};

static float window_width, window_height;
static bool window_changed = false;

void error_callback(int err, const char *desc)
{
    fprintf(stderr, "GLFW error %i: %s\n", err, desc);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    aspect_ratio = (float)width / (float)height;
    glViewport(0, 0, width, height);
}

void window_size_callback(GLFWwindow *window, int width, int height)
{
    window_width = width;
    window_height = height;
    window_changed = true;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
	if(action == GLFW_PRESS)
	{
	    mouse_pressed = true;
	}
	else if(action == GLFW_RELEASE)
	{
	    mouse_pressed = false;
	}
    }
}

void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos)
{
    mouse_pos.x = xpos;
    mouse_pos.y = ypos;
    mouse_moved = true;
}

void scroll_callback(GLFWwindow *window, double x_scroll, double y_scroll)
{
    scale -= 0.1*scale*y_scroll;
}


int main()
{
    // Set error callback before doing anything
    glfwSetErrorCallback(error_callback);

    // initialize
    if(!glfwInit())
    {
	return 1;
    }
    defer { glfwTerminate(); };

    // Verify runtime version matches
    //
    int major, minor, revision;
    glfwGetVersion(&major, &minor, &revision);

    if(major != GLFW_VERSION_MAJOR)
    {
	fprintf(stderr, "Running on GLFW version '%s'. Major version %i required.\n",
		glfwGetVersionString(), GLFW_VERSION_MAJOR);
	return 1;
    }
    if(minor < GLFW_VERSION_MINOR)
    {
	fprintf(stderr, "Running on GLFW version '%s'. At least minor version %i.%i required.\n",
		glfwGetVersionString(), GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR);
	return 1;
    }

    // Create GLFW window
    //
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    
    GLFWwindow *window = glfwCreateWindow(480, 480, "The Mandelbrot Set", NULL, NULL);
    if(!window)
    {
	fprintf(stderr, "Unable to create window with desired properties.\n");
	return 1;
    }

    // Load OpenGL functions
    //
    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
	fprintf(stderr, "Unable to load OpenGL\n");
	return 1;
    }

    // Get initial values, in the case they aren't updated before we use them
    {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	mouse_pos.x = xpos;
	mouse_pos.y = ypos;

	int w_width, w_height;
	glfwGetWindowSize(window, &w_width, &w_height);
	window_width = w_width;
	window_height = w_height;
    }

    
    // Set window callbacks
    //
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);

    // Get OpenCL platforms
    cl_uint n_platforms;
    cl_int ret = clGetPlatformIDs(0, nullptr, &n_platforms);

    if(n_platforms == 0)
    {
	fprintf(stderr, "No OpenCL platforms!\n");
	return 1;
    }

    // Use the first platform for now
    cl_platform_id platform;

    clGetPlatformIDs(1, &platform, nullptr);

    // Get GPU device ids
    //
    cl_uint n_gpus = 0;
    ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &n_gpus);
    if(ret == CL_DEVICE_NOT_FOUND)
    {
	fprintf(stderr, "No GPU's on the first OpenCL platform\n");
	return 1;
    }

    cl_device_id *gpus = (cl_device_id*) malloc(sizeof(cl_device_id) * n_gpus);
    defer { free(gpus); };

    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, n_gpus, gpus, nullptr);

    // Create OpenCL context
    cl_context context = clCreateContext(nullptr, n_gpus, gpus, nullptr, nullptr, &ret);
    if(ret == CL_DEVICE_NOT_AVAILABLE)
    {
	fprintf(stderr, "GPU's are not available\n");
	return 1;
    }
    else if(ret == CL_OUT_OF_HOST_MEMORY)
    {
	fprintf(stderr, "OpenCL didn't have enough memory\n");
	return 1;
    }
    else if(ret != CL_SUCCESS)
    {
	fprintf(stderr, "Unable to create context. Error code %i\n", ret);
	return 1;
    }

    defer { clReleaseContext(context); };

    // Create OpenCL command queues
    //
    cl_command_queue *command_queues = (cl_command_queue*) malloc(n_gpus * sizeof(cl_command_queue));

    for(int i = 0; i < n_gpus; ++i)
    {
	command_queues[i] = clCreateCommandQueue(context, gpus[i], CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &ret);
	
	if(ret == CL_INVALID_QUEUE_PROPERTIES)
	{
	    command_queues[i] = clCreateCommandQueue(context, gpus[i], 0, &ret);
	}
	if(ret == CL_OUT_OF_HOST_MEMORY)
	{
	    fprintf(stderr, "OpenCL ran out of memory\n");
	}
	else if(ret != CL_SUCCESS)
	{
	    fprintf(stderr, "Unable to create command queues. Error code %i\n", ret);
	}
	if(ret != CL_SUCCESS)
	{
	    for(int j = 0; j < i; ++j)
	    {
		clReleaseCommandQueue(command_queues[j]);
	    }
	    free(command_queues);
	    return 1;
	}
    }

    defer {
	for(int i = 0; i < n_gpus; ++i)
	{
	    clReleaseCommandQueue(command_queues[i]);
	}
	free(command_queues);
    };

    // Load OpenCL kernel
    cl_kernel kernel;
    if(!load_kernel(context, n_gpus, gpus, "gpu_programs/test.cl", "test_kernel", kernel))
    {
	return 1;
    }

    GLuint program_id;
    if(!load_shader_program("gpu_programs/picture.vert", "gpu_programs/picture.frag", &program_id))
    {
	return 1;
    }

    // Setup rendering data
    //
    GLuint vertex_array_id;
    glGenVertexArrays(1, &vertex_array_id);
    glBindVertexArray(vertex_array_id);

    static const GLfloat vertex_data[] = {
	-1,-1,
	1, -1,
	1,  1,
	-1,-1,
	1,  1,
	-1, 1
    };
    
    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);


    
    // TODO from here down is incomplete

    
    cl_image_format img_format = {CL_RGBA, CL_UNORM_INT8};
    cl_image_desc img_desc = {
	CL_MEM_OBJECT_IMAGE2D,
	IMAGE_SIZE,
	IMAGE_SIZE,
	1,
	1,
	0,
	0,
	0,
	0,
	nullptr
    };
    
    cl_mem memory = clCreateImage(context, CL_MEM_WRITE_ONLY, &img_format, &img_desc, nullptr, &ret);
    if(ret != CL_SUCCESS)
    {
	fprintf(stderr, "Unable to create OpenCL image\n");
	return 1;
    }
    defer { clReleaseMemObject(memory); };

    float step = 4.0f / ((float)IMAGE_SIZE);
    
    cl_float2 origin = {-2,-2};
    cl_float2 dx = {step, 0};
    cl_float2 dy = {0, step};
    
    clSetKernelArg(kernel, 0, sizeof(cl_float2), &origin);
    clSetKernelArg(kernel, 1, sizeof(cl_float2), &dx);
    clSetKernelArg(kernel, 2, sizeof(cl_float2), &dy);
	    
    ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&memory);
    if(ret != CL_SUCCESS)
    {
	fprintf(stderr, "Unable to set kernel argument\n");
	return 1;
    }

    printf("Enqueuing task\n");
    
    for(int i = 0; i < n_gpus; ++i)
    {
	const static size_t work_sizes[] = {IMAGE_SIZE, IMAGE_SIZE};
	ret = clEnqueueNDRangeKernel(command_queues[i], kernel, 2, nullptr, work_sizes, nullptr, 0, nullptr, nullptr);
	if(ret != CL_SUCCESS)
	{
	    fprintf(stderr, "Unable to enqueue task\n");
	    return 1;
	}
    }

    int *buffer = (int*) malloc(IMAGE_SIZE*IMAGE_SIZE*sizeof(int));

    printf("Enqueuing read\n");
    
    for(int i = 0; i < n_gpus; ++i)
    {
	const static size_t read_origin[] = {0,0,0};
	const static size_t read_region[] = {IMAGE_SIZE,IMAGE_SIZE,1};
	ret = clEnqueueReadImage(command_queues[i], memory, CL_TRUE, read_origin, read_region, 0, 0, buffer, 0, nullptr, nullptr);
	if(ret != CL_SUCCESS)
	{
	    fprintf(stderr, "Unable to read buffer\n");
	    return 1;
	}
    }

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, IMAGE_SIZE, IMAGE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    free(buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    while(!glfwWindowShouldClose(window))
    {
	auto start = std::chrono::high_resolution_clock::now();

	glfwPollEvents();

	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program_id);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(0);
	
	glfwSwapBuffers(window);

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = end-start;
	auto wait_time = std::chrono::milliseconds(33) - duration;
	if(wait_time >= std::chrono::seconds(0))
	{
	    std::this_thread::sleep_for(wait_time);
	}
    }
    
    return 0;
}
