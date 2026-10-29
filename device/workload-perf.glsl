/* workload_perf shaders, one file, stages and variants selected with -D
 * (see workload-perf-build-spv.sh). */
#version 450
#extension GL_ARB_shader_viewport_layer_array : enable

#if defined(VS) || defined(VS_XFB)
layout(location = 0) in vec4 a;
layout(push_constant) uniform PC { vec4 p; } pc;
layout(location = 0) out vec4 vcol;
#ifdef VS_XFB
layout(xfb_buffer = 0, xfb_offset = 0, xfb_stride = 16, location = 1) out vec4 so;
#endif
void main()
{
   gl_Position = vec4(a.xy + pc.p.zw, 0.0, 1.0);
   vcol = vec4(fract(a.z * 0.37 + pc.p.x), 1.0 - fract(a.z * 0.11), 0.25, 1.0);
#ifdef VS_XFB
   so = gl_Position;
#endif
}
#endif

#ifdef VS_FULL
/* Full-screen triangle, no vertex input; uv for the texture cases. */
layout(location = 0) out vec4 vcol;
void main()
{
   vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
   gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
   vcol = vec4(p, 0.5, 0.6);
}
#endif

#if defined(GS) || defined(GS_LAYER) || defined(GS_XFB)
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
layout(location = 0) in vec4 vcol_in[];
layout(location = 0) out vec4 vcol;
#ifdef GS_XFB
layout(xfb_buffer = 0, xfb_offset = 0, xfb_stride = 16, location = 1) out vec4 so;
#endif
void main()
{
   for (int i = 0; i < 3; i++) {
      gl_Position = gl_in[i].gl_Position;
      vcol = vcol_in[i];
#ifdef GS_LAYER
      gl_Layer = 0;
#endif
#ifdef GS_XFB
      so = gl_in[i].gl_Position;
#endif
      EmitVertex();
   }
}
#endif

#ifdef FS
layout(location = 0) in vec4 vcol;
layout(location = 0) out vec4 o;
void main() { o = vcol; }
#endif

#ifdef FS_TEX
/* uv runs over [-0.25, 1.25]: the outer band samples the border colour. */
layout(set = 0, binding = 0) uniform sampler2D tex;
layout(location = 0) in vec4 vcol;
layout(location = 0) out vec4 o;
void main() { o = texture(tex, vcol.xy * 1.5 - 0.25); }
#endif
