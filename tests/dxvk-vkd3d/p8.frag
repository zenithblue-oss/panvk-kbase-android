#version 460
layout(set = 0, binding = 0, std430) buffer Out { uint value[4]; } outbuf;
layout(location = 0) out vec4 color;
void main()
{
   atomicAdd(outbuf.value[3], 1u);
   color = vec4(0.25, 0.5, 0.75, 1.0);
}
