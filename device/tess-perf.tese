#version 450
/* tess_perf domain: quads, fractional_odd (common in D3D11 terrain), ccw so
 * the winding-swap path runs. */
layout(quads, fractional_odd_spacing, ccw) in;
layout(location = 0) out vec4 vcol;

void
main()
{
   float u = gl_TessCoord.x, v = gl_TessCoord.y;
   vec2 p0 = mix(gl_in[0].gl_Position.xy, gl_in[1].gl_Position.xy, u);
   vec2 p1 = mix(gl_in[3].gl_Position.xy, gl_in[2].gl_Position.xy, u);
   vec2 pos = mix(p0, p1, v);
   gl_Position = vec4(pos, 0.0, 1.0);
   /* Not affine in (u, v), and with the level in blue: on these flat
    * quads a linear colour gives the same image at every tess level, so
    * the A/B hash could not see a level or domain-point bug. */
   vcol = vec4(u * u, v * v,
               0.5 * float(gl_PrimitiveID & 15) / 15.0 +
                  gl_TessLevelInner[0] / 64.0,
               1.0);
}
