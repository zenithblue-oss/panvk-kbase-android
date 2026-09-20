#version 450

layout(push_constant) uniform Push { uint word[64]; } pc;
layout(location = 0) flat in uint vertex_signature;
layout(location = 0) out vec4 color;
layout(set = 0, binding = 0) buffer Data { uint value[]; } data[32];

void main()
{
   uint base = pc.word[2] * 4;
   data[0].value[base] = vertex_signature;
   data[0].value[base + 1] = pc.word[0] ^ pc.word[31] ^ pc.word[47] ^
                               pc.word[62] ^ pc.word[63];
   data[0].value[base + 2] = data[1].value[0] ^ data[31].value[0];
   color = vec4(0.0, 1.0, 0.0, 1.0);
}
