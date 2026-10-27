// PanPlay D3D feature-level tests (d3d12fl.c, d3d11fl.c): shader model 5.0 shaders, one entry point each, plus
// the SM4-safe ones from dfl4.hlsl. Precompiled to DXBC by build-dfl.sh (vkd3d-compiler). PanVK project code, MIT.
#include "dfl4.hlsl"

// FL11_1: UAV write from the vertex shader (one atomic add per vertex), full-screen triangle.
RWStructuredBuffer<uint> vs_counter : register(u1);
float4 vs_uav(uint vid : SV_VertexID) : SV_Position
{
    uint o;
    InterlockedAdd(vs_counter[0], 1, o);
    float2 uv = float2((vid << 1) & 2, vid & 2);
    return float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}

// ROV test: full-screen triangle per instance, instance id to the pixel shader.
struct VInst { float4 pos : SV_Position; nointerpolation uint inst : INST; };
VInst vs_inst(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    VInst o;
    float2 uv = float2((vid << 1) & 2, vid & 2);
    o.pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    o.inst = iid;
    return o;
}

// Tessellation input: control points from the constants (not SV_Position).
struct VCtl { float4 pos : CTLPOS; };
VCtl vs_ctl(uint vid : SV_VertexID)
{
    VCtl o;
    o.pos = float4(p[vid].xy, 0, 1);
    return o;
}

// Tessellation (FL11_0): tri patch, factor 4; the domain shader lifts interior points by 8*u*v*w, so a
// degenerate (zero-area) patch only covers pixels when the tessellator really generated interior vertices.
struct HSConst { float e[3] : SV_TessFactor; float i : SV_InsideTessFactor; };
HSConst hs_const(InputPatch<VCtl, 3> ip)
{
    HSConst o;
    o.e[0] = 4; o.e[1] = 4; o.e[2] = 4;
    o.i = 4;
    return o;
}
[domain("tri")] [partitioning("integer")] [outputtopology("triangle_cw")] [outputcontrolpoints(3)]
[patchconstantfunc("hs_const")]
VCtl hs_main(InputPatch<VCtl, 3> ip, uint i : SV_OutputControlPointID)
{
    return ip[i];
}
[domain("tri")]
float4 ds_bulge(HSConst c, float3 b : SV_DomainLocation, const OutputPatch<VCtl, 3> op) : SV_Position
{
    float4 q = op[0].pos * b.x + op[1].pos * b.y + op[2].pos * b.z;
    q.y += 8.0 * b.x * b.y * b.z;
    return q;
}

struct MrtOut { float4 a : SV_Target0; float4 b : SV_Target1; };
MrtOut ps_mrt(float4 pos : SV_Position)
{
    MrtOut o;
    o.a = float4(1, 0, 0, 1);
    o.b = float4(0, 0, 1, 1);
    return o;
}

// FL11_1 logic op target (R8G8B8A8_UINT).
uint4 ps_uint(float4 pos : SV_Position) : SV_Target
{
    return uint4(0xff, 0x0f, 0xf0, 0xaa);
}

// Target-independent rasterization and conservative raster: count pixel shader invocations, no render target.
RWStructuredBuffer<uint> ps_counter : register(u1);
void ps_count(float4 pos : SV_Position)
{
    uint o;
    InterlockedAdd(ps_counter[0], 1, o);
}

// FL12_1 ROVs: ordered read-modify-write per pixel; 3 instances give ((0*3+1)*3+2)*3+3 = 18 only in order.
RasterizerOrderedTexture2D<uint> rov : register(u1);
float4 ps_rov(VInst i) : SV_Target
{
    uint2 q = uint2(i.pos.xy);
    rov[q] = rov[q] * 3 + i.inst + 1;
    return float4(0, 1, 0, 1);
}

RWStructuredBuffer<uint> cs_out : register(u0);
[numthreads(64, 1, 1)]
void cs_write(uint3 id : SV_DispatchThreadID)
{
    cs_out[id.x] = id.x * 3 + 1;
}

// FL12_0 typed UAV load (R8G8B8A8_UNORM): read, scale, write back.
RWTexture2D<float4> cs_img : register(u0);
[numthreads(8, 8, 1)]
void cs_typed(uint3 id : SV_DispatchThreadID)
{
    cs_img[id.xy] = cs_img[id.xy] * 2.0 + float4(0.125, 0.25, 0.0625, 0.0);
}

// Binding: SRV t0 and UAVs u0 and u63 from descriptor tables at the end of a 1,000,000-entry heap.
Buffer<uint> bind_src : register(t0);
RWBuffer<uint> bind_u0 : register(u0);
RWBuffer<uint> bind_u63 : register(u63);
[numthreads(1, 1, 1)]
void cs_bind(uint3 id : SV_DispatchThreadID)
{
    bind_u0[0] = bind_src[0] + 1;
    bind_u63[0] = bind_src[0] + 2;
}

// FL12_0 tiled resources tier 2: write to a mapped and to an unmapped 64 KiB tile of a reserved buffer.
RWByteAddressBuffer tiled_buf : register(u0);
[numthreads(1, 1, 1)]
void cs_tiled(uint3 id : SV_DispatchThreadID)
{
    tiled_buf.Store(0, 0x1234);
    tiled_buf.Store(65536, 0x5678);
}
