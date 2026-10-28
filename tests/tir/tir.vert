#version 450
/* Full-width band y in [10.05, 10.45] px of a 32x32 viewport: covers the 4x
 * sample rows 10.125 and 10.375 but no pixel centre (10.5). */
void main()
{
   float x = (gl_VertexIndex & 1) != 0 ? 1.0 : -1.0;
   float y = ((gl_VertexIndex & 2) != 0 ? 10.45 : 10.05) / 16.0 - 1.0;
   gl_Position = vec4(x, y, 0.0, 1.0);
}
