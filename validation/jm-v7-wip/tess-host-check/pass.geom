#version 450
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
layout(location=0) in vec4 t_col[];
layout(location=0) out vec4 g_col;
void main() {
   for (int i = 0; i < 3; i++) {
      gl_Position = gl_in[i].gl_Position;
      g_col = t_col[i] + vec4(gl_PrimitiveIDIn);
      EmitVertex();
   }
   EndPrimitive();
}
