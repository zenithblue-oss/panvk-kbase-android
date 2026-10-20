#version 450
layout(triangles, equal_spacing, cw) in;
layout(location=0) in vec4 c_col[];
layout(location=0, xfb_buffer=0, xfb_offset=16) out vec4 t_col;
out gl_PerVertex { layout(xfb_buffer=0, xfb_offset=0) vec4 gl_Position; };
void main() {
   gl_Position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;
   t_col = c_col[0];
}
