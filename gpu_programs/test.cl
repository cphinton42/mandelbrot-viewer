__kernel void test_kernel(float2 origin, float2 dx, float2 dy, image2d_t img)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int2 pos = (int2)(x,y);
    
    float2 c = origin + ((float)x)*dx + ((float)y)*dy;

    float2 z = (float2)(0,0);

    for(int i = 0; i < 100; ++i)
    {
	z = (float2) ((z.x+z.y)*(z.x-z.y), 2*z.x*z.y);
	z = z + c;
	if(dot(z,z) > 4)
	{
	    float x = (float)i / (float)100;
	    float4 color = (float4)(x,x,x,1.0);
	    write_imagef(img, pos, color);
	    return;
	}
    }

    write_imagef(img, pos, (float4)(0,0,0,1));
}
