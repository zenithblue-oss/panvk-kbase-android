#version 450
layout(quads, fractional_odd_spacing, cw) in;
layout(location=0) in vec4 c_col[];
layout(location=0) out vec4 t_col;
void main() {
   vec4 a = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
   vec4 b = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
   gl_Position = mix(a, b, gl_TessCoord.y);
   t_col = c_col[gl_PatchVerticesIn - 1] + vec4(gl_TessLevelInner[1]);
}
