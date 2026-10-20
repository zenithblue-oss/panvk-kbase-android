#version 450
layout(location=0) out vec4 v_col;
layout(location=1) out float v_extra[2];
void main() {
   vec2 p = vec2(gl_VertexIndex & 1, (gl_VertexIndex >> 1) & 1);
   gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0 + float(gl_InstanceIndex));
   v_col = vec4(p, 0.0, 1.0);
   v_extra[0] = float(gl_VertexIndex);
   v_extra[1] = 2.0;
}
