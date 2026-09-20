#version 450

layout(push_constant) uniform Push { uint word[64]; } pc;
layout(location = 0) flat out uint vertex_signature;

void main()
{
   const vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0),
                                     vec2(-1.0, 3.0));
   gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
   vertex_signature = pc.word[0] ^ pc.word[31] ^ pc.word[47] ^
                      pc.word[62] ^ pc.word[63];
}
