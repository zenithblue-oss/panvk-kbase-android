#version 450
layout(set = 0, binding = 0) buffer B { uint n[4]; };
layout(push_constant) uniform P { uint i; };
void main()
{
   atomicAdd(n[i], 1u);
}
