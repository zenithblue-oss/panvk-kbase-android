#version 450
/* tess_perf hull: the tess factor comes from the per-draw push constant
 * (pc.p.x), as a DXVK hull shader reads it from a constant buffer. With
 * pc.p.y != 0 it varies per patch over 1..16 around it (distance-LOD-like). */
layout(vertices = 4) out;
layout(push_constant) uniform PC { vec4 p; } pc;

void
main()
{
   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
   if (gl_InvocationID == 0) {
      float l = pc.p.x;
      if (pc.p.y != 0.0)
         l = float((gl_PrimitiveID * 7 + int(l)) % 16 + 1);
      gl_TessLevelOuter[0] = l;
      gl_TessLevelOuter[1] = l;
      gl_TessLevelOuter[2] = l;
      gl_TessLevelOuter[3] = l;
      gl_TessLevelInner[0] = l;
      gl_TessLevelInner[1] = l;
   }
}
