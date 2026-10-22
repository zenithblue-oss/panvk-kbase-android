#version 450

/* vs_img_redraw: a vertex shader that writes a storage image. The test draws
 * it several times with no rebind in between; every draw must reach the
 * image (PanVK v6/v7 rebuilt the VS image attribute table per draw but only
 * filled it when the descriptors were dirty, jm-v7 090).
 */
layout(set = 0, binding = 0, r32ui) uniform writeonly uimage2D img;

void main()
{
   int idx = gl_VertexIndex;
   imageStore(img, ivec2(idx, 0), uvec4(uint(idx) + 1u));

   gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
   gl_PointSize = 1.0;
}
