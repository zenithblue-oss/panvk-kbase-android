#version 450
/* tess_perf GS-on variant: pass-through, as DXVK's emulated SO/viewport GS. */
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
layout(location = 0) in vec4 vcol_in[];
layout(location = 0) out vec4 vcol;

void
main()
{
   for (int i = 0; i < 3; i++) {
      gl_Position = gl_in[i].gl_Position;
      vcol = vcol_in[i];
      EmitVertex();
   }
}
