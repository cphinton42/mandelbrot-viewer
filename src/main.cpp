#include <chrono>
#include <cstdio>
#include <thread>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define GLM_FORCE_CXX11 
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "glad/glad.c"

#include "typedefs.h"
#include "defer.h"


static const char *vertex_shader_src =
    "#version 330 core\n"
    "layout(location = 0) in vec3 v_pos;"
    "uniform mat3 view_matrix;"
    "out vec2 m_pos;"
    "void main() {"
    "gl_Position = vec4(v_pos.xy, 0, 1);"
    "m_pos = (view_matrix*v_pos).xy;"
    "}";

static const char *fragment_shader_src =
    "#version 330 core\n"
    "in vec2 m_pos;"
    "out vec3 color;"
    "vec2 c_sqr(in vec2 z) {"
    " return vec2(dot(z,vec2(z.x,-z.y)),2*z.x*z.y);"
    "}"
    "void main() {"
    " vec2 z = vec2(0,0);"
    " for(int i = 0; i < 100; ++i) {"
    "  z = c_sqr(z) + m_pos;"
    "  if((z.x*z.x + z.y*z.y) > 4) {"
    "   color = (float(i) / 100.0) * vec3(1,1,1);"
    "   return;"
    "  }"
    " }"
    " color = vec3(0,0,0);"
    "}";

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
    else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
	printf("scale: %.9g\n", scale);
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

    // Compile shaders
    //
    GLuint program_id;
    {
	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

	defer {
	    glDeleteShader(vertex_shader_id);
	    glDeleteShader(fragment_shader_id);
	};
	
	GLint compile_success = GL_FALSE;

	glShaderSource(vertex_shader_id, 1, &vertex_shader_src, nullptr);
	glCompileShader(vertex_shader_id);
	
	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &compile_success);
	if(!compile_success)
	{
	    int info_log_len;
	    glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_len);
	    char *buffer = (char*)malloc(info_log_len);
	    glGetShaderInfoLog(vertex_shader_id, info_log_len, nullptr, buffer);
	    fprintf(stderr, "Error compiling vertex shader:\n%s\n", buffer);
	    return 1;
	}
	
	glShaderSource(fragment_shader_id, 1, &fragment_shader_src, nullptr);
	glCompileShader(fragment_shader_id);
	
	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &compile_success);
	if(!compile_success)
	{
	    int info_log_len;
	    glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_len);
	    char *buffer = (char*)malloc(info_log_len);
	    glGetShaderInfoLog(vertex_shader_id, info_log_len, nullptr, buffer);
	    fprintf(stderr, "Error compiling fragment shader:\n%s\n", buffer);
	    return 1;	
	}
	
	program_id = glCreateProgram();
	
	glAttachShader(program_id, vertex_shader_id);
	glAttachShader(program_id, fragment_shader_id);

	defer {
	    glDetachShader(program_id, vertex_shader_id);
	    glDetachShader(program_id, fragment_shader_id);
	};
	
	glLinkProgram(program_id);
	
	glGetProgramiv(program_id, GL_LINK_STATUS, &compile_success);
	if(!compile_success)
	{
	    int info_log_len;
	    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_len);
	    char *buffer = (char*)malloc(info_log_len);
	    glGetShaderInfoLog(program_id, info_log_len, nullptr, buffer);
	    fprintf(stderr, "Error linking shader program:\n%s\n", buffer);
	    return 1;	
	}
    }

    // Setup rendering data
    //
    GLuint vertex_array_id;
    glGenVertexArrays(1, &vertex_array_id);
    glBindVertexArray(vertex_array_id);

    static const GLfloat vertex_data[] = {
	-1,-1, 1,
	1, -1, 1,
	1,  1, 1,
	-1,-1, 1,
	1,  1, 1,
	-1, 1, 1
    };
    
    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);

    GLint matrix_id = glGetUniformLocation(program_id, "view_matrix");
    
    glm::mat3 view_matrix = {2, 0, 0,
			     0, 2, 0,
			     0, 0, 1};
    
    glm::mat3 window_matrix = {2/window_width, 0, 0,
			       0, -2/window_height, 0,
			       -1, 1, 1};
    
    float center_x = 0;
    float center_y = 0;

    glm::vec3 prev_mouse_pos = mouse_pos;
    
    // Main loop!
    //
    while(!glfwWindowShouldClose(window))
    {
	auto start = std::chrono::high_resolution_clock::now();

	glfwPollEvents();

	if(window_changed)
	{
	    window_matrix[0][0] = 2/window_width;
	    window_matrix[1][1] = -2/window_height;
	    
	    window_changed = false;
	}

	if(mouse_moved)
	{
	    if(mouse_pressed)
	    {
		glm::vec3 prev = view_matrix * (window_matrix * prev_mouse_pos);
		glm::vec3 current = view_matrix * (window_matrix * mouse_pos);

		glm::vec3 dp = current - prev;
		center_x -= dp.x;
		center_y -= dp.y;
	    }
	    prev_mouse_pos = mouse_pos;
	    mouse_moved = false;
	}

	float half_h = 2*scale;
	float half_w = aspect_ratio * half_h;
	view_matrix[0][0] = half_w;
	view_matrix[2][0] = center_x;
	view_matrix[1][1] = half_h;
	view_matrix[2][1] = center_y;

	
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program_id);

	glUniformMatrix3fv(matrix_id, 1, GL_FALSE, glm::value_ptr(view_matrix));
	
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLES, 0, 12*3);

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
