#version 450
layout(vertices = 3) out;
layout(location=0) in vec4 v_col[];
layout(location=1) in float v_extra[][2];
layout(location=0) out vec4 c_col[];
layout(location=1) patch out vec4 c_patch;
layout(push_constant) uniform PC { float level; int idx; } pc;
void main() {
   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
   c_col[gl_InvocationID] = v_col[gl_InvocationID] + vec4(v_extra[gl_InvocationID][pc.idx & 1]);
   barrier();
   if (gl_InvocationID == 0) {
      gl_TessLevelOuter[0] = pc.level;
      gl_TessLevelOuter[1] = pc.level;
      gl_TessLevelOuter[2] = pc.level;
      gl_TessLevelInner[0] = pc.level;
      c_patch = c_col[2] + vec4(gl_PrimitiveID, gl_PatchVerticesIn, 0, 0);
   }
}
