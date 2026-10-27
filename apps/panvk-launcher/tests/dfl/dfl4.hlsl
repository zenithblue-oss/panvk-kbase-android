// PanPlay D3D feature-level tests: the shaders that also build for shader model 4.x (D3D11 FL10_0/10_1 runs).
// dfl.hlsl includes this file and adds the SM5 shaders. Precompiled to DXBC by build-dfl.sh (vkd3d-compiler).
// PanVK project code, MIT.
// Registers are the same in D3D11 and D3D12: b0 = 4 float4 constants, t0 = SRV, s0 = sampler,
// u0 = CS UAV, u1 = VS/PS UAV (D3D11 binds PS/VS UAVs after the one render target).

cbuffer CB : register(b0) { float4 p[4]; };

// Full-screen triangle.
float4 vs_full(uint vid : SV_VertexID) : SV_Position
{
    float2 uv = float2((vid << 1) & 2, vid & 2);
    return float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}

// Vertex vid from the constants (xy = NDC position).
float4 vs_cb(uint vid : SV_VertexID) : SV_Position
{
    return float4(p[vid].xy, 0, 1);
}

// Present loop: a triangle turning by p[0].x radians, colour p[1].
struct VCol { float4 pos : SV_Position; float4 col : COLOR; };
VCol vs_spin(uint vid : SV_VertexID)
{
    VCol o;
    float a = p[0].x + vid * 2.0943951;
    o.pos = float4(0.7 * sin(a), 0.7 * cos(a), 0, 1);
    o.col = p[1] * (0.6 + 0.2 * vid);
    o.col.a = 1;
    return o;
}

// Geometry shader (FL10_0, also used at 11_0): one point in, a full-screen quad (strip of 4) out.
struct GOut { float4 pos : SV_Position; };
[maxvertexcount(4)]
void gs_quad(point GOut i[1], inout TriangleStream<GOut> s)
{
    GOut o;
    o.pos = float4(-1, -1, 0, 1); s.Append(o);
    o.pos = float4(-1, 1, 0, 1); s.Append(o);
    o.pos = float4(1, -1, 0, 1); s.Append(o);
    o.pos = float4(1, 1, 0, 1); s.Append(o);
}

float4 ps_green(float4 pos : SV_Position) : SV_Target
{
    return float4(0, 1, 0, 1);
}

float4 ps_vcol(VCol i) : SV_Target
{
    return i.col;
}

// FL10_1: sample slice 1 of a cube array (+X face), and Gather4 of a 2x2 R8 texture.
TextureCubeArray<float4> cube_arr : register(t0);
Texture2D<float4> tex2d : register(t0);
SamplerState smp : register(s0);
float4 ps_cubearr(float4 pos : SV_Position) : SV_Target
{
    return cube_arr.SampleLevel(smp, float4(1, 0, 0, 1), 0);
}
float4 ps_gather(float4 pos : SV_Position) : SV_Target
{
    return tex2d.Gather(smp, float2(0.5, 0.5));
}
