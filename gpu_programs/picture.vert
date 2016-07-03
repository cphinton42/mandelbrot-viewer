#version 330 core

layout(location = 0) in vec2 v_pos;

out vec2 t_pos;

void main()
{
    gl_Position = vec4(v_pos, 0, 1);
    t_pos = (v_pos + vec2(1,1)) / 2;
}
