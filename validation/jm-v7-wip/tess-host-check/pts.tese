#version 450
layout(triangles, point_mode) in;
layout(location=0) in vec4 c_col[];
layout(location=0) out vec4 t_col;
void main() {
   gl_Position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;
   t_col = c_col[0];
}
