#version 450
/* tess_perf: control point = a.xy, shifted per draw by pc.p.zw. REF draws
 * the same points as a plain triangle list (per-draw baseline). */
layout(location = 0) in vec4 a;
layout(push_constant) uniform PC { vec4 p; } pc;
#ifdef REF
layout(location = 0) out vec4 vcol;
#endif

void
main()
{
   gl_Position = vec4(a.xy + pc.p.zw, 0.0, 1.0);
#ifdef REF
   vcol = vec4(0.0, 1.0, 0.0, 1.0);
#endif
}
