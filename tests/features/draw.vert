#version 460
layout(location = 0) out vec4 color;
layout(push_constant) uniform Params { uint mode; } pc;
void main()
{
   const vec2 tri[3] = vec2[3](vec2(-0.8, -0.8), vec2(0.8, -0.8), vec2(0.0, 0.8));
   const vec2 line[2] = vec2[2](vec2(-0.75, 0.0), vec2(0.75, 0.0));
   vec2 p = pc.mode == 0u ? tri[gl_VertexIndex] :
            pc.mode == 1u ? line[gl_VertexIndex] : vec2(0.0);
   gl_Position = vec4(p, 0.0, 1.0);
   gl_PointSize = pc.mode == 2u ? 11.0 : 1.0;
   color = pc.mode == 0u ? vec4(1, 0, 0, 1) :
           pc.mode == 1u ? vec4(0, 1, 0, 1) : vec4(1, 1, 0, 1);
}
