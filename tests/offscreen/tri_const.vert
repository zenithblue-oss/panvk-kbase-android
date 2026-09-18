#version 450
void main() {
  vec2 p0 = vec2(-1.0, -1.0);
  vec2 p1 = vec2(-0.25, -1.0);
  vec2 p2 = vec2(-1.0, -0.25);
  vec2 p = (gl_VertexIndex == 0) ? p0 : ((gl_VertexIndex == 1) ? p1 : p2);
  gl_Position = vec4(p, 0.0, 1.0);
}
