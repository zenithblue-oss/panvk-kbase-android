#version 460
#extension GL_ARB_shader_draw_parameters : require
layout(set = 0, binding = 0, std430) buffer Out { uint value[4]; } outbuf;
layout(push_constant) uniform Push { float z; } push;
void main()
{
   const vec2 pos[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
   outbuf.value[2] = uint(gl_BaseInstanceARB);
   gl_Position = vec4(pos[gl_VertexIndex], push.z, 1.0);
}
