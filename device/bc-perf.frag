#version 450
/* lod < 0: four implicit-LOD taps (cache friendly). lod >= 0: one tap at a
 * forced LOD (minified without mips: bandwidth bound). */
layout(set = 0, binding = 0) uniform sampler2D tex;
layout(push_constant) uniform P {
    vec4 m;
    vec2 o;
    float lod;
} p;
layout(location = 0) out vec4 color;

void main()
{
    vec2 uv = gl_FragCoord.xy * (1.0 / 2048.0);
    vec2 t = vec2(dot(p.m.xy, uv), dot(p.m.zw, uv)) + p.o;
    if (p.lod >= 0.0) {
        color = textureLod(tex, t, p.lod);
        return;
    }
    const float d = 1.0 / 1024.0;
    vec4 a = texture(tex, t);
    a += texture(tex, t + vec2(d, 0.0));
    a += texture(tex, t + vec2(0.0, d));
    a += texture(tex, t + vec2(d, d));
    color = a * 0.25;
}
