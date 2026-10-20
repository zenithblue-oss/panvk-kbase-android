#version 450
layout(isolines, fractional_even_spacing) in;
layout(location=0) in vec4 c_col[];
layout(location=0) out vec4 t_col;
void main() {
   gl_Position = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x) + vec4(0, gl_TessCoord.y, 0, 0);
   t_col = c_col[1];
}
