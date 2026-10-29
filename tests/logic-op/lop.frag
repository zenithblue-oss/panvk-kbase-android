#version 450
layout(location = 0) out uvec4 o;
void main()
{
   o = uvec4(0xff, 0x0f, 0xf0, 0xaa);
}
