#version 450
#extension GL_ARB_enhanced_layouts : enable
#if MODE == 2
#extension GL_ARB_shader_draw_parameters : require
#endif

#if MODE == 0
/* Captures (vertex index, instance index) at stride 16. */
layout(location = 0, xfb_buffer = 0, xfb_offset = 0, xfb_stride = 16) out vec4 out_val;
void main() {
   gl_Position = vec4(0.0);
   out_val = vec4(float(gl_VertexIndex), float(gl_InstanceIndex), 0.0, 1.0);
}
#elif MODE == 2
/* Captures (vertex index, instance index, per-instance attribute,
 * gl_BaseInstance + 1). The attribute buffer holds element e = e. */
layout(location = 0) in float a_inst;
layout(location = 0, xfb_buffer = 0, xfb_offset = 0, xfb_stride = 16) out vec4 out_val;
void main() {
   gl_Position = vec4(0.0);
   out_val = vec4(float(gl_VertexIndex), float(gl_InstanceIndex), a_inst,
                  float(gl_BaseInstanceARB + 1));
}
#else
/* Feeds the geometry and tessellation shaders. */
layout(location = 0) out vec4 out_v;
void main() {
   gl_Position = vec4(0.0);
   out_v = vec4(float(gl_VertexIndex), float(gl_InstanceIndex), 0.0, 1.0);
}
#endif
