#version 330 core

in vec2 m_pos;
out vec3 color;

vec2 c_sqr(in vec2 z)
{
    return vec2(dot(z,vec2(z.x,-z.y)),2*z.x*z.y);
}

void main()
{
    vec2 z = vec2(0,0);
    for(int i = 0; i < 100; ++i)
    {
	z = c_sqr(z) + m_pos;
	if((z.x*z.x + z.y*z.y) > 4)
	{
	    color = (float(i) / 100.0) * vec3(1,1,1);
	    return;
	}
    }
    color = vec3(0,0,0);
}
