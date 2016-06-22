#version 330 core

layout(location = 0) in vec3 v_pos;
uniform mat3 view_matrix;

out vec2 m_pos;

void main()
{
    gl_Position = vec4(v_pos.xy, 0, 1);
    m_pos = (view_matrix*v_pos).xy;
}
