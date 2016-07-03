#version 330 core

in vec2 t_pos;
out vec3 color;

uniform sampler2D texture_sampler;

void main()
{
    color = texture(texture_sampler, t_pos).rgb;
}
