#include <cstdio>
#include <cstring>

#include "defer.h"
#include "load_shader.h"
#include "glad/glad.h"

bool load_shader_program(const char *vertex_file, const char *fragment_file, GLuint *id_out)
{
    FILE *vert_shader_file = fopen(vertex_file, "r");
    if(!vert_shader_file)
    {
	fprintf(stderr, "Unable to open file '%s': %s\n", vertex_file, std::strerror(errno));
	return false;
    }
    defer { fclose(vert_shader_file); };
    
    FILE *frag_shader_file = fopen(fragment_file, "r");
    if(!frag_shader_file)
    {
	fprintf(stderr, "Unable to open file '%s': %s\n", fragment_file, std::strerror(errno));
	return false;
    }
    defer { fclose(frag_shader_file); };

    GLuint vert_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    
    defer {
	glDeleteShader(vert_shader_id);
	glDeleteShader(frag_shader_id);
    };
    
    char buffer[8192];
    // frankly, I don't know why this is necessary, the compiler complained
    const char *buffer_ptr = buffer;
    
    while(!feof(vert_shader_file))
    {
	size_t n_read = fread(buffer, 1, 8192, vert_shader_file);
	if(ferror(vert_shader_file))
	{
	    fprintf(stderr, "Unable to read file '%s'\n", vertex_file);
	    return false;
	}
	GLint len = n_read;
	glShaderSource(vert_shader_id, 1, &buffer_ptr, &len);
    }
    while(!feof(frag_shader_file))
    {
	size_t n_read = fread(buffer, 1, 8192, frag_shader_file);
	if(ferror(frag_shader_file))
	{
	    fprintf(stderr, "Unable to read file '%s'\n", fragment_file);
	    return false;
	}
	GLint len = n_read;
	glShaderSource(frag_shader_id, 1, &buffer_ptr, &len);
    }


    glCompileShader(vert_shader_id);
    glCompileShader(frag_shader_id);
    
    GLint compile_success;
    glGetShaderiv(vert_shader_id, GL_COMPILE_STATUS, &compile_success);
    
    if(!compile_success)
    {
	GLint info_log_len;
	glGetShaderiv(vert_shader_id, GL_INFO_LOG_LENGTH, &info_log_len);

	char *log_buffer = buffer;
	auto free_buffer = maybe { free(log_buffer); }; 
	if(info_log_len > 8192)
	{
	    char *log_buffer = (char*)malloc(info_log_len);
	    free_buffer.activate();
	}

	glGetShaderInfoLog(vert_shader_id, info_log_len, nullptr, log_buffer);
	
	fprintf(stderr, "Error compiling vertex shader:\n%s\n", log_buffer);
	return false;
    }

    glGetShaderiv(frag_shader_id, GL_COMPILE_STATUS, &compile_success);
    
    if(!compile_success)
    {
	GLint info_log_len;
	glGetShaderiv(frag_shader_id, GL_INFO_LOG_LENGTH, &info_log_len);

	char *log_buffer = buffer;
	auto free_buffer = maybe { free(log_buffer); }; 
	if(info_log_len > 8192)
	{
	    char *log_buffer = (char*)malloc(info_log_len);
	    free_buffer.activate();
	}

	glGetShaderInfoLog(frag_shader_id, info_log_len, nullptr, log_buffer);
	
	fprintf(stderr, "Error compiling fragment shader:\n%s\n", log_buffer);
	return false;
    }
	
    GLuint program_id = glCreateProgram();
	
    glAttachShader(program_id, vert_shader_id);
    glAttachShader(program_id, frag_shader_id);

    defer {
	glDetachShader(program_id, vert_shader_id);
	glDetachShader(program_id, frag_shader_id);
    };
	
    glLinkProgram(program_id);

    glGetProgramiv(program_id, GL_LINK_STATUS, &compile_success);

    if(!compile_success)
    {
	GLint info_log_len;
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_len);

	char *log_buffer = buffer;
	auto free_buffer = maybe { free(log_buffer); }; 
	if(info_log_len > 8192)
	{
	    char *log_buffer = (char*)malloc(info_log_len);
	    free_buffer.activate();
	}

	glGetShaderInfoLog(program_id, info_log_len, nullptr, log_buffer);
	
	fprintf(stderr, "Error linking shader program:\n%s\n", log_buffer);
	return false;
    }

    *id_out = program_id;
    return true;
}
