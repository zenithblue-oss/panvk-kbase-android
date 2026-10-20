#version 450
layout(vertices = 4) out;
layout(location=0) in vec4 v_col[];
layout(location=0) out vec4 c_col[];
void main() {
   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
   c_col[gl_InvocationID] = v_col[gl_InvocationID];
   gl_TessLevelOuter[0] = 3.0; gl_TessLevelOuter[1] = 5.0;
   gl_TessLevelOuter[2] = 7.0; gl_TessLevelOuter[3] = 2.0;
   gl_TessLevelInner[0] = 6.0; gl_TessLevelInner[1] = 4.0;
}
