/* PanPlay D3D12 feature-level test (vkd3d-proton). Creates the device at each Direct3D 12 feature level, draws or
 * dispatches the level's defining features, checks them by readback, then renders for -secs N seconds.
 *   11_0: tessellation, geometry shader, compute, MRT, 4x MSAA resolve
 *   11_1: UAV in the vertex shader, logic ops, target-independent rasterization (informational: vkd3d-proton
 *         2230755 ignores ForcedSampleCount, libs/vkd3d/state.c FIXME; d3d11fl tests TIR through DXVK)
 *   12_0: typed UAV loads, resource binding tier 2 (1,000,000-descriptor heap, u63), tiled resources tier 2
 *   12_1: ROVs, conservative rasterization
 *   12_2: caps only (SM 6.5, mesh shaders, DXR 1.1, VRS tier 2, sampler feedback)
 * A level the device cannot be created at is UNSUPPORTED; its features still run on the highest-level device.
 * Usage: d3d12fl.exe [11_0|11_1|12_0|12_1|12_2|all] [-secs N] [-sync N]. Output: dfl_common.h.
 * Shaders: dfl.hlsl, precompiled into dfl_shaders.h by build-dfl.sh. PanVK project code, MIT. */
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "dfl_common.h"
#include "dfl_shaders.h"

#define W 64 /* test targets are W x W */
#define RGBA8 DXGI_FORMAT_R8G8B8A8_UNORM
#define UA D3D12_RESOURCE_STATE_UNORDERED_ACCESS
#define RTS D3D12_RESOURCE_STATE_RENDER_TARGET
#define SH(n) ((D3D12_SHADER_BYTECODE){ n##_dxbc, sizeof(n##_dxbc) })
#define RET(st, ...) do { snprintf(d, 300, __VA_ARGS__); return (st); } while (0)
/* a feature without its own cap bit: missing is a FAIL only when the device was created at the level */
#define BAD (created ? DFL_FAIL : DFL_UNSUP)
#define PR(v) ((int)((v) & 0xff))
#define PG(v) ((int)(((v) >> 8) & 0xff))
#define PB(v) ((int)(((v) >> 16) & 0xff))
#define PA(v) ((int)((v) >> 24))
#define CENTER px[W / 2 * W + W / 2]

static const char *const LV[] = { "11_0", "11_1", "12_0", "12_1", "12_2" };
static const D3D_FEATURE_LEVEL FL[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_12_0,
                                        D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_2 };

static ID3D12Device *dev;
static ID3D12CommandQueue *queue;
static ID3D12CommandAllocator *alloc;
static ID3D12GraphicsCommandList *list;
static ID3D12Fence *fence;
static HANDLE fence_ev;
static UINT64 fence_val;
static ID3D12RootSignature *rs;           /* b0 = 16 root constants (float4 p[4]), table [u0 u1 t0] */
static ID3D12DescriptorHeap *heap, *rtvs; /* shader-visible [u0 u1 t0]; 8 RTVs (4, 5: swapchain) */
static UINT inc, rtv_inc;
static D3D12_FEATURE_DATA_D3D12_OPTIONS opt;
static int lost, created;                 /* created: the device can be created at the level under test */
static IUnknown *objs[16], *junk[16];     /* per-test objects; upload buffers (freed after each submit) */
static int nobj, njunk;
static unsigned px[W * W];
static const float zero16[16];

static void *keep(void *p)
{
    if (p && nobj < 16) objs[nobj++] = p;
    return p;
}

static void exec(void)
{
    ID3D12CommandList *l = (ID3D12CommandList *)list;
    if (SUCCEEDED(ID3D12GraphicsCommandList_Close(list))) ID3D12CommandQueue_ExecuteCommandLists(queue, 1, &l);
    else lost = 1;
}

static int wait_idle(void)
{
    ID3D12CommandQueue_Signal(queue, fence, ++fence_val);
    if (ID3D12Fence_GetCompletedValue(fence) < fence_val) {
        ID3D12Fence_SetEventOnCompletion(fence, fence_val, fence_ev);
        if (WaitForSingleObject(fence_ev, 20000) != WAIT_OBJECT_0) lost = 1;
    }
    if (FAILED(ID3D12Device_GetDeviceRemovedReason(dev))) lost = 1;
    while (!lost && njunk) {
        IUnknown *o = junk[--njunk];
        IUnknown_Release(o);
    }
    return lost ? -1 : 0;
}

static int submit(void)
{
    exec();
    return wait_idle();
}

static void begin(void)
{
    ID3D12CommandAllocator_Reset(alloc);
    ID3D12GraphicsCommandList_Reset(list, alloc, NULL);
}

static ID3D12Resource *mkres(D3D12_HEAP_TYPE type, const D3D12_RESOURCE_DESC *rd, D3D12_RESOURCE_STATES st)
{
    D3D12_HEAP_PROPERTIES hp;
    ID3D12Resource *r = NULL;
    memset(&hp, 0, sizeof(hp));
    hp.Type = type;
    if (FAILED(ID3D12Device_CreateCommittedResource(dev, &hp, D3D12_HEAP_FLAG_NONE, rd, st, NULL, &IID_ID3D12Resource,
                                                    (void **)&r)))
        return NULL;
    return r;
}

static D3D12_RESOURCE_DESC bufdesc(UINT64 size, D3D12_RESOURCE_FLAGS f)
{
    D3D12_RESOURCE_DESC rd;
    memset(&rd, 0, sizeof(rd));
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = size;
    rd.Height = 1;
    rd.DepthOrArraySize = rd.MipLevels = 1;
    rd.SampleDesc.Count = 1;
    rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rd.Flags = f;
    return rd;
}

static ID3D12Resource *mkbuf(UINT64 size, D3D12_HEAP_TYPE type, D3D12_RESOURCE_FLAGS f)
{
    D3D12_RESOURCE_DESC rd = bufdesc(size, f);
    return mkres(type, &rd, type == D3D12_HEAP_TYPE_UPLOAD ? D3D12_RESOURCE_STATE_GENERIC_READ
                            : type == D3D12_HEAP_TYPE_READBACK ? D3D12_RESOURCE_STATE_COPY_DEST
                                                               : D3D12_RESOURCE_STATE_COMMON);
}

/* w x w 2D texture */
static ID3D12Resource *mktex(DXGI_FORMAT fmt, UINT w, UINT samples, D3D12_RESOURCE_FLAGS f, D3D12_RESOURCE_STATES st)
{
    D3D12_RESOURCE_DESC rd;
    memset(&rd, 0, sizeof(rd));
    rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rd.Width = w;
    rd.Height = w;
    rd.DepthOrArraySize = rd.MipLevels = 1;
    rd.Format = fmt;
    rd.SampleDesc.Count = samples;
    rd.Flags = f;
    return mkres(D3D12_HEAP_TYPE_DEFAULT, &rd, st);
}

static void barrier(ID3D12Resource *r, D3D12_RESOURCE_STATES a, D3D12_RESOURCE_STATES b)
{
    D3D12_RESOURCE_BARRIER br;
    if (a == b) return;
    memset(&br, 0, sizeof(br));
    br.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    br.Transition.pResource = r;
    br.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    br.Transition.StateBefore = a;
    br.Transition.StateAfter = b;
    ID3D12GraphicsCommandList_ResourceBarrier(list, 1, &br);
}

static UINT pitch(UINT w, UINT bpp) { return (w * bpp + 255) & ~255u; }

static void loc_fp(D3D12_TEXTURE_COPY_LOCATION *l, ID3D12Resource *b, DXGI_FORMAT fmt, UINT w, UINT bpp)
{
    memset(l, 0, sizeof(*l));
    l->pResource = b;
    l->Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    l->PlacedFootprint.Footprint.Format = fmt;
    l->PlacedFootprint.Footprint.Width = w;
    l->PlacedFootprint.Footprint.Height = w;
    l->PlacedFootprint.Footprint.Depth = 1;
    l->PlacedFootprint.Footprint.RowPitch = pitch(w, bpp);
}

static void loc_sub(D3D12_TEXTURE_COPY_LOCATION *l, ID3D12Resource *t)
{
    memset(l, 0, sizeof(*l));
    l->pResource = t;
    l->Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
}

/* Upload buffer holding data (or `size` bytes of `fill`), released after the next submit. */
static ID3D12Resource *upload(const void *data, int fill, UINT size)
{
    ID3D12Resource *u = mkbuf(size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
    void *m;
    if (!u) return NULL;
    if (njunk >= 16 || FAILED(ID3D12Resource_Map(u, 0, NULL, &m))) {
        ID3D12Resource_Release(u);
        return NULL;
    }
    if (data) memcpy(m, data, size);
    else memset(m, fill, size);
    ID3D12Resource_Unmap(u, 0, NULL);
    junk[njunk++] = (IUnknown *)u;
    return u;
}

/* Records: buffer b (COMMON) <- data, then b to state st. */
static int put_buf(ID3D12Resource *b, const void *data, UINT size, D3D12_RESOURCE_STATES st)
{
    ID3D12Resource *u = upload(data, 0, size);
    if (!u) return -1;
    ID3D12GraphicsCommandList_CopyBufferRegion(list, b, 0, u, 0, size);
    barrier(b, D3D12_RESOURCE_STATE_COPY_DEST, st);
    return 0;
}

/* Records: texture t (COPY_DEST, w x w, bpp bytes per texel) filled with byte `fill`, then t to state st. */
static int put_tex(ID3D12Resource *t, DXGI_FORMAT fmt, UINT w, UINT bpp, int fill, D3D12_RESOURCE_STATES st)
{
    D3D12_TEXTURE_COPY_LOCATION s, dl;
    ID3D12Resource *u = upload(NULL, fill, pitch(w, bpp) * w);
    if (!u) return -1;
    loc_fp(&s, u, fmt, w, bpp);
    loc_sub(&dl, t);
    ID3D12GraphicsCommandList_CopyTextureRegion(list, &dl, 0, 0, 0, &s, NULL);
    barrier(t, D3D12_RESOURCE_STATE_COPY_DEST, st);
    return 0;
}

/* Copies texture t (state st, w x w) to out (tight rows), submits and waits. 0 = ok. */
static int read_tex(ID3D12Resource *t, D3D12_RESOURCE_STATES st, DXGI_FORMAT fmt, UINT w, UINT bpp, void *out)
{
    UINT p = pitch(w, bpp);
    ID3D12Resource *rb = mkbuf((UINT64)p * w, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE);
    D3D12_TEXTURE_COPY_LOCATION s, dl;
    unsigned char *m;
    int r = -1;
    if (rb) {
        barrier(t, st, D3D12_RESOURCE_STATE_COPY_SOURCE);
        loc_sub(&s, t);
        loc_fp(&dl, rb, fmt, w, bpp);
        ID3D12GraphicsCommandList_CopyTextureRegion(list, &dl, 0, 0, 0, &s, NULL);
    }
    if (!submit() && rb && SUCCEEDED(ID3D12Resource_Map(rb, 0, NULL, (void **)&m))) {
        for (UINT y = 0; y < w; y++) memcpy((unsigned char *)out + y * w * bpp, m + y * p, w * bpp);
        ID3D12Resource_Unmap(rb, 0, NULL);
        r = 0;
    }
    if (rb) ID3D12Resource_Release(rb);
    return r;
}

static int read_buf(ID3D12Resource *b, D3D12_RESOURCE_STATES st, UINT size, void *out)
{
    ID3D12Resource *rb = mkbuf(size, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE);
    void *m;
    int r = -1;
    if (rb) {
        barrier(b, st, D3D12_RESOURCE_STATE_COPY_SOURCE);
        ID3D12GraphicsCommandList_CopyBufferRegion(list, rb, 0, b, 0, size);
    }
    if (!submit() && rb && SUCCEEDED(ID3D12Resource_Map(rb, 0, NULL, &m))) {
        memcpy(out, m, size);
        ID3D12Resource_Unmap(rb, 0, NULL);
        r = 0;
    }
    if (rb) ID3D12Resource_Release(rb);
    return r;
}

static D3D12_CPU_DESCRIPTOR_HANDLE cpuh(ID3D12DescriptorHeap *h, UINT i, UINT step)
{
    D3D12_CPU_DESCRIPTOR_HANDLE c = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(h);
    c.ptr += (SIZE_T)i * step;
    return c;
}

static D3D12_GPU_DESCRIPTOR_HANDLE gpuh(ID3D12DescriptorHeap *h, UINT i)
{
    D3D12_GPU_DESCRIPTOR_HANDLE g = ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(h);
    g.ptr += (UINT64)i * inc;
    return g;
}

/* Buffer UAV at slot i: fmt UNKNOWN = structured (stride 4), R32_TYPELESS = raw, else typed; r NULL = null view. */
static void uav_buf(ID3D12DescriptorHeap *h, UINT i, ID3D12Resource *r, DXGI_FORMAT fmt, UINT first, UINT n)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC u;
    memset(&u, 0, sizeof(u));
    u.Format = fmt;
    u.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    u.Buffer.FirstElement = first;
    u.Buffer.NumElements = n;
    u.Buffer.StructureByteStride = fmt == DXGI_FORMAT_UNKNOWN ? 4 : 0;
    u.Buffer.Flags = fmt == DXGI_FORMAT_R32_TYPELESS ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
    ID3D12Device_CreateUnorderedAccessView(dev, r, NULL, &u, cpuh(h, i, inc));
}

static void uav_tex(UINT i, ID3D12Resource *t, DXGI_FORMAT fmt)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC u;
    memset(&u, 0, sizeof(u));
    u.Format = fmt;
    u.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    ID3D12Device_CreateUnorderedAccessView(dev, t, NULL, &u, cpuh(heap, i, inc));
}

/* R32_UINT buffer SRV, one element */
static void srv_buf(ID3D12DescriptorHeap *h, UINT i, ID3D12Resource *r)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC s;
    memset(&s, 0, sizeof(s));
    s.Format = DXGI_FORMAT_R32_UINT;
    s.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    s.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    s.Buffer.NumElements = 1;
    ID3D12Device_CreateShaderResourceView(dev, r, &s, cpuh(h, i, inc));
}

static void null_table(void)
{
    uav_buf(heap, 0, NULL, DXGI_FORMAT_R32_UINT, 0, 1);
    uav_buf(heap, 1, NULL, DXGI_FORMAT_R32_UINT, 0, 1);
    srv_buf(heap, 2, NULL);
}

static ID3D12RootSignature *mkrs(const D3D12_ROOT_PARAMETER *p, UINT n)
{
    D3D12_ROOT_SIGNATURE_DESC d;
    ID3DBlob *b = NULL, *e = NULL;
    ID3D12RootSignature *r = NULL;
    memset(&d, 0, sizeof(d));
    d.NumParameters = n;
    d.pParameters = p;
    if (SUCCEEDED(D3D12SerializeRootSignature(&d, D3D_ROOT_SIGNATURE_VERSION_1, &b, &e)))
        ID3D12Device_CreateRootSignature(dev, 0, ID3D10Blob_GetBufferPointer(b), ID3D10Blob_GetBufferSize(b),
                                         &IID_ID3D12RootSignature, (void **)&r);
    if (b) ID3D10Blob_Release(b);
    if (e) ID3D10Blob_Release(e);
    return r;
}

static void gdesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC *g, D3D12_SHADER_BYTECODE vs, D3D12_SHADER_BYTECODE ps,
                  DXGI_FORMAT rt)
{
    const D3D12_DEPTH_STENCILOP_DESC so = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
                                            D3D12_COMPARISON_FUNC_ALWAYS };
    memset(g, 0, sizeof(*g));
    g->pRootSignature = rs;
    g->VS = vs;
    g->PS = ps;
    for (int i = 0; i < 8; i++) {
        D3D12_RENDER_TARGET_BLEND_DESC *b = &g->BlendState.RenderTarget[i];
        b->SrcBlend = b->SrcBlendAlpha = D3D12_BLEND_ONE;
        b->DestBlend = b->DestBlendAlpha = D3D12_BLEND_ZERO;
        b->BlendOp = b->BlendOpAlpha = D3D12_BLEND_OP_ADD;
        b->LogicOp = D3D12_LOGIC_OP_NOOP;
        b->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    g->SampleMask = ~0u;
    g->RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    g->RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    g->RasterizerState.DepthClipEnable = TRUE;
    g->DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    g->DepthStencilState.FrontFace = g->DepthStencilState.BackFace = so;
    g->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    g->NumRenderTargets = rt != DXGI_FORMAT_UNKNOWN;
    g->RTVFormats[0] = rt;
    g->SampleDesc.Count = 1;
}

static ID3D12PipelineState *gpso(const D3D12_GRAPHICS_PIPELINE_STATE_DESC *g, HRESULT *hr)
{
    ID3D12PipelineState *p = NULL;
    *hr = ID3D12Device_CreateGraphicsPipelineState(dev, g, &IID_ID3D12PipelineState, (void **)&p);
    return SUCCEEDED(*hr) ? p : NULL;
}

static ID3D12PipelineState *cpso(ID3D12RootSignature *r, D3D12_SHADER_BYTECODE cs, HRESULT *hr)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC c;
    ID3D12PipelineState *p = NULL;
    memset(&c, 0, sizeof(c));
    c.pRootSignature = r;
    c.CS = cs;
    *hr = ID3D12Device_CreateComputePipelineState(dev, &c, &IID_ID3D12PipelineState, (void **)&p);
    return SUCCEEDED(*hr) ? p : NULL;
}

static ID3D12Resource *mkrt(DXGI_FORMAT fmt, UINT samples, UINT slot, D3D12_CPU_DESCRIPTOR_HANDLE *h)
{
    ID3D12Resource *t = mktex(fmt, W, samples, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, RTS);
    *h = cpuh(rtvs, slot, rtv_inc);
    if (t) ID3D12Device_CreateRenderTargetView(dev, t, NULL, *h);
    return keep(t);
}

static void clear(D3D12_CPU_DESCRIPTOR_HANDLE h, float r, float g, float b, float a)
{
    const float c[4] = { r, g, b, a };
    ID3D12GraphicsCommandList_ClearRenderTargetView(list, h, c, 0, NULL);
}

static void gfx(ID3D12PipelineState *p, const D3D12_CPU_DESCRIPTOR_HANDLE *rt, UINT nrt, UINT w, UINT h,
                const float *c, D3D_PRIMITIVE_TOPOLOGY topo)
{
    D3D12_VIEWPORT vp = { 0, 0, (float)w, (float)h, 0, 1 };
    D3D12_RECT sc = { 0, 0, (LONG)w, (LONG)h };
    ID3D12GraphicsCommandList_SetDescriptorHeaps(list, 1, &heap);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(list, rs);
    ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(list, 0, 16, c ? c : zero16, 0);
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(list, 1, gpuh(heap, 0));
    ID3D12GraphicsCommandList_SetPipelineState(list, p);
    ID3D12GraphicsCommandList_RSSetViewports(list, 1, &vp);
    ID3D12GraphicsCommandList_RSSetScissorRects(list, 1, &sc);
    ID3D12GraphicsCommandList_OMSetRenderTargets(list, nrt, nrt ? rt : NULL, FALSE, NULL);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(list, topo);
}

static void comp(ID3D12PipelineState *p)
{
    ID3D12GraphicsCommandList_SetDescriptorHeaps(list, 1, &heap);
    ID3D12GraphicsCommandList_SetComputeRootSignature(list, rs);
    ID3D12GraphicsCommandList_SetComputeRoot32BitConstants(list, 0, 16, zero16, 0);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(list, 1, gpuh(heap, 0));
    ID3D12GraphicsCommandList_SetPipelineState(list, p);
}

static int count_green(void)
{
    int n = 0;
    for (int i = 0; i < W * W; i++) n += PG(px[i]) > 200 && PR(px[i]) < 60;
    return n;
}

static int within2(int a, int b) { return abs(a - b) <= 2; }

/* One draw without render targets counting pixel shader invocations in u1 (ps_count). */
static int count_draw(ID3D12PipelineState *p, const float *c, UINT nv, D3D_PRIMITIVE_TOPOLOGY topo,
                      ID3D12Resource *cnt, unsigned *out)
{
    unsigned z = 0;
    begin();
    if (put_buf(cnt, &z, 4, UA)) return submit(), -1;
    null_table();
    uav_buf(heap, 1, cnt, DXGI_FORMAT_UNKNOWN, 0, 1);
    gfx(p, NULL, 0, W, W, c, topo);
    ID3D12GraphicsCommandList_DrawInstanced(list, nv, 1, 0, 0);
    return read_buf(cnt, UA, 4, out);
}

/* ---- 11_0 ---- */

static int t_tess(char *d)
{
    const float c[16] = { -0.9f, 0, 0, 0, 0.9f, 0, 0, 0, 0, 0, 0, 0 }; /* zero-area patch on y = 0 */
    D3D12_GRAPHICS_PIPELINE_STATE_DESC g;
    D3D12_CPU_DESCRIPTOR_HANDLE h;
    ID3D12PipelineState *p;
    ID3D12Resource *rt;
    HRESULT hr;
    int n;
    gdesc(&g, SH(vs_ctl), SH(ps_green), RGBA8);
    g.HS = SH(hs_main);
    g.DS = SH(ds_bulge);
    g.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    if (!(p = keep(gpso(&g, &hr)))) RET(DFL_FAIL, "HS/DS pipeline hr=0x%08lx", (unsigned long)hr);
    if (!(rt = mkrt(RGBA8, 1, 0, &h))) RET(DFL_FAIL, "render target");
    begin();
    clear(h, 0, 0, 0, 1);
    gfx(p, &h, 1, W, W, c, D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    ID3D12GraphicsCommandList_DrawInstanced(list, 3, 1, 0, 0);
    if (read_tex(rt, RTS, RGBA8, W, 4, px)) RET(DFL_FAIL, "submit/readback failed");
    n = count_green();
    RET(n >= 20 ? DFL_PASS : DFL_FAIL, "zero-area tri patch, factor 4, interior lifted by the DS: %d px covered (want >= 20)", n);
}

static int t_gs(char *d)
{
    const float c[16] = { -0.95f, -0.95f };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC g;
    D3D12_CPU_DESCRIPTOR_HANDLE h;
    ID3D12PipelineState *p;
    ID3D12Resource *rt;
    HRESULT hr;
    int n;
    gdesc(&g, SH(vs_cb), SH(ps_green), RGBA8);
    g.GS = SH(gs_quad);
    g.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    if (!(p = keep(gpso(&g, &hr)))) RET(DFL_FAIL, "GS pipeline hr=0x%08lx", (unsigned long)hr);
    if (!(rt = mkrt(RGBA8, 1, 0, &h))) RET(DFL_FAIL, "render target");
    begin();
    clear(h, 0, 0, 0, 1);
    gfx(p, &h, 1, W, W, c, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    ID3D12GraphicsCommandList_DrawInstanced(list, 1, 1, 0, 0);
    if (read_tex(rt, RTS, RGBA8, W, 4, px)) RET(DFL_FAIL, "submit/readback failed");
    n = count_green();
    RET(n >= W * W * 9 / 10 ? DFL_PASS : DFL_FAIL, "one point in, full-screen strip out: %d/%d px covered", n, W * W);
}

static int t_compute(char *d)
{
    unsigned v[64];
    ID3D12Resource *b = keep(mkbuf(256, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
    ID3D12PipelineState *p;
    HRESULT hr;
    int bad = 0;
    if (!(p = keep(cpso(rs, SH(cs_write), &hr)))) RET(DFL_FAIL, "CS pipeline hr=0x%08lx", (unsigned long)hr);
    if (!b) RET(DFL_FAIL, "buffer");
    begin();
    null_table();
    uav_buf(heap, 0, b, DXGI_FORMAT_UNKNOWN, 0, 64);
    barrier(b, D3D12_RESOURCE_STATE_COMMON, UA);
    comp(p);
    ID3D12GraphicsCommandList_Dispatch(list, 1, 1, 1);
    if (read_buf(b, UA, sizeof(v), v)) RET(DFL_FAIL, "submit/readback failed");
    for (unsigned i = 0; i < 64; i++) bad += v[i] != i * 3 + 1;
    RET(!bad ? DFL_PASS : DFL_FAIL, "64 threads wrote i*3+1 to a structured UAV: %d wrong (v[63]=%u)", bad, v[63]);
}

static int t_mrt(char *d)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC g;
    D3D12_CPU_DESCRIPTOR_HANDLE h[2];
    ID3D12Resource *t0 = mkrt(RGBA8, 1, 0, &h[0]), *t1 = mkrt(RGBA8, 1, 1, &h[1]);
    ID3D12PipelineState *p;
    HRESULT hr;
    unsigned a, b;
    gdesc(&g, SH(vs_full), SH(ps_mrt), RGBA8);
    g.NumRenderTargets = 2;
    g.RTVFormats[1] = RGBA8;
    if (!(p = keep(gpso(&g, &hr)))) RET(DFL_FAIL, "2-target pipeline hr=0x%08lx", (unsigned long)hr);
    if (!t0 || !t1) RET(DFL_FAIL, "render targets");
    begin();
    clear(h[0], 0, 0, 0, 1);
    clear(h[1], 0, 0, 0, 1);
    gfx(p, h, 2, W, W, NULL, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_DrawInstanced(list, 3, 1, 0, 0);
    if (read_tex(t0, RTS, RGBA8, W, 4, px)) RET(DFL_FAIL, "submit/readback failed");
    a = CENTER;
    begin();
    if (read_tex(t1, RTS, RGBA8, W, 4, px)) RET(DFL_FAIL, "submit/readback failed");
    b = CENTER;
    RET(a == 0xff0000ffu && b == 0xffff0000u ? DFL_PASS : DFL_FAIL,
        "SV_Target0=%08x (want ff0000ff), SV_Target1=%08x (want ffff0000)", a, b);
}

static int t_msaa(char *d)
{
    const float c[16] = { -1, -1, 0, 0, -1, 1, 0, 0, 1, 1 }; /* upper-left half, diagonal through pixel centres */
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS q = { RGBA8, 4, D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE, 0 };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC g;
    D3D12_CPU_DESCRIPTOR_HANDLE h;
    ID3D12PipelineState *p;
    ID3D12Resource *ms, *res;
    HRESULT hr;
    int part = 0, full = 0;
    if (FAILED(ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &q, sizeof(q))) ||
        !q.NumQualityLevels)
        RET(DFL_FAIL, "R8G8B8A8_UNORM 4x: NumQualityLevels=0");
    gdesc(&g, SH(vs_cb), SH(ps_green), RGBA8);
    g.SampleDesc.Count = 4;
    if (!(p = keep(gpso(&g, &hr)))) RET(DFL_FAIL, "4x pipeline hr=0x%08lx", (unsigned long)hr);
    ms = mkrt(RGBA8, 4, 0, &h);
    res = keep(mktex(RGBA8, W, 1, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_RESOLVE_DEST));
    if (!ms || !res) RET(DFL_FAIL, "4x render target");
    begin();
    clear(h, 0, 0, 0, 1);
    gfx(p, &h, 1, W, W, c, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_DrawInstanced(list, 3, 1, 0, 0);
    barrier(ms, RTS, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
    ID3D12GraphicsCommandList_ResolveSubresource(list, res, 0, ms, 0, RGBA8);
    if (read_tex(res, D3D12_RESOURCE_STATE_RESOLVE_DEST, RGBA8, W, 4, px)) RET(DFL_FAIL, "submit/readback failed");
    for (int i = 0; i < W * W; i++) {
        part += PG(px[i]) >= 40 && PG(px[i]) <= 215;
        full += PG(px[i]) > 215;
    }
    RET(part >= 16 && full >= 1000 ? DFL_PASS : DFL_FAIL,
        "resolved: %d edge px partly covered (want >= 16), %d fully (want >= 1000)", part, full);
}

/* ---- 11_1 ---- */

static int t_uav_vs(char *d)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC g;
    D3D12_CPU_DESCRIPTOR_HANDLE h;
    ID3D12Resource *cnt = keep(mkbuf(4, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
    ID3D12Resource *rt = mkrt(RGBA8, 1, 0, &h);
    ID3D12PipelineState *p;
    HRESULT hr;
    unsigned v = 0;
    gdesc(&g, SH(vs_uav), SH(ps_green), RGBA8);
    if (!(p = keep(gpso(&g, &hr)))) RET(BAD, "pipeline with a VS UAV: hr=0x%08lx", (unsigned long)hr);
    if (!cnt || !rt) RET(DFL_FAIL, "resources");
    begin();
    if (put_buf(cnt, &v, 4, UA)) return submit(), snprintf(d, 300, "upload"), DFL_FAIL;
    null_table();
    uav_buf(heap, 1, cnt, DXGI_FORMAT_UNKNOWN, 0, 1);
    clear(h, 0, 0, 0, 1);
    gfx(p, &h, 1, W, W, NULL, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_DrawInstanced(list, 3, 1, 0, 0);
    if (read_buf(cnt, UA, 4, &v)) RET(DFL_FAIL, "submit/readback failed");
    RET(v >= 3 ? DFL_PASS : BAD, "VS atomics on u1: %u (want >= 3)", v);
}

/* Plain draw of (ff,0f,f0,aa), then a logic-op INVERT draw: ~ = (00,f0,0f,55). Independent of integer clears. */
static int t_logic_op(char *d)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC g;
    D3D12_CPU_DESCRIPTOR_HANDLE h;
    ID3D12Resource *rt;
    ID3D12PipelineState *pc, *pi;
    HRESULT hr;
    if (!opt.OutputMergerLogicOp) RET(DFL_UNSUP, "OutputMergerLogicOp=0");
    gdesc(&g, SH(vs_full), SH(ps_uint), DXGI_FORMAT_R8G8B8A8_UINT);
    pc = keep(gpso(&g, &hr));
    g.BlendState.RenderTarget[0].LogicOpEnable = TRUE;
    g.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_INVERT;
    if (!(pi = keep(gpso(&g, &hr))) || !pc) RET(DFL_FAIL, "logic op pipeline hr=0x%08lx", (unsigned long)hr);
    if (!(rt = mkrt(DXGI_FORMAT_R8G8B8A8_UINT, 1, 0, &h))) RET(DFL_FAIL, "render target");
    begin();
    clear(h, 0, 0, 0, 0);
    gfx(pc, &h, 1, W, W, NULL, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_DrawInstanced(list, 3, 1, 0, 0);
    ID3D12GraphicsCommandList_SetPipelineState(list, pi);
    ID3D12GraphicsCommandList_DrawInstanced(list, 3, 1, 0, 0);
    if (read_tex(rt, RTS, DXGI_FORMAT_R8G8B8A8_UINT, W, 4, px)) RET(DFL_FAIL, "submit/readback failed");
    RET(CENTER == 0x550ff000u ? DFL_PASS : DFL_FAIL, "INVERT after writing aaf00fff: %08x (want 550ff000)", CENTER);
}

/* Informational (vkd3d-proton ignores ForcedSampleCount): a band y in [10.05, 10.45] px covers the 4x sample rows
 * 10.125 and 10.375 but no pixel centre, so only target-independent rasterization runs the pixel shader. */
static int t_tir(char *d)
{
    const float y0 = 1 - 10.05f / 32, y1 = 1 - 10.45f / 32;
    const float c[16] = { -1, y0, 0, 0, 1, y0, 0, 0, -1, y1, 0, 0, 1, y1, 0, 0 };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC g;
    ID3D12Resource *cnt = keep(mkbuf(4, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
    ID3D12PipelineState *p0, *p4;
    HRESULT hr;
    unsigned n0 = 0, n4 = 0;
    gdesc(&g, SH(vs_cb), SH(ps_count), DXGI_FORMAT_UNKNOWN);
    p0 = keep(gpso(&g, &hr));
    g.RasterizerState.ForcedSampleCount = 4;
    if (!(p4 = keep(gpso(&g, &hr))) || !p0) RET(DFL_REPORTED, "pipeline hr=0x%08lx", (unsigned long)hr);
    if (!cnt || count_draw(p0, c, 4, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, cnt, &n0) ||
        count_draw(p4, c, 4, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, cnt, &n4))
        RET(DFL_FAIL, "submit/readback failed");
    RET(DFL_REPORTED, "ForcedSampleCount=4: %u PS invocations, 0: %u, TIR %s (vkd3d-proton ignores ForcedSampleCount; "
        "d3d11fl checks TIR through DXVK)", n4, n0, n4 > n0 ? "applied" : "not applied");
}

/* ---- 12_0 ---- */

static int t_typed_uav(char *d)
{
    D3D12_FEATURE_DATA_FORMAT_SUPPORT fs = { RGBA8, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
    ID3D12Resource *t;
    ID3D12PipelineState *p;
    HRESULT hr;
    unsigned v[64];
    int bad = 0;
    ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_FORMAT_SUPPORT, &fs, sizeof(fs));
    if (!opt.TypedUAVLoadAdditionalFormats || !(fs.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD))
        RET(DFL_UNSUP, "TypedUAVLoadAdditionalFormats=%d, R8G8B8A8_UNORM UAV_TYPED_LOAD=%d",
            opt.TypedUAVLoadAdditionalFormats, !!(fs.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD));
    if (!(p = keep(cpso(rs, SH(cs_typed), &hr)))) RET(DFL_FAIL, "CS pipeline hr=0x%08lx", (unsigned long)hr);
    if (!(t = keep(mktex(RGBA8, 8, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST))))
        RET(DFL_FAIL, "texture");
    begin();
    if (put_tex(t, RGBA8, 8, 4, 0x40, UA)) return submit(), snprintf(d, 300, "upload"), DFL_FAIL;
    null_table();
    uav_tex(0, t, RGBA8);
    comp(p);
    ID3D12GraphicsCommandList_Dispatch(list, 1, 1, 1);
    if (read_tex(t, UA, RGBA8, 8, 4, v)) RET(DFL_FAIL, "submit/readback failed");
    for (int i = 0; i < 64; i++)
        bad += !within2(PR(v[i]), 160) || !within2(PG(v[i]), 192) || !within2(PB(v[i]), 144) || !within2(PA(v[i]), 128);
    RET(!bad ? DFL_PASS : DFL_FAIL, "RWTexture2D<float4> R8G8B8A8_UNORM load*2+bias: %08x (want ~8090c0a0), %d/64 wrong",
        v[0], bad);
}

static int t_binding(char *d)
{
    D3D12_DESCRIPTOR_HEAP_DESC hd = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000000,
                                      D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
    D3D12_DESCRIPTOR_RANGE rg[2] = { { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 64, 0, 0, 0 },
                                     { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0 } };
    D3D12_ROOT_PARAMETER rp[2];
    ID3D12DescriptorHeap *big = NULL;
    ID3D12RootSignature *r2;
    ID3D12Resource *sb, *ub;
    ID3D12PipelineState *p;
    HRESULT hr;
    unsigned v[2] = { 0, 0 }, src = 41;
    if (opt.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_2)
        RET(DFL_UNSUP, "ResourceBindingTier=%d (want >= 2)", opt.ResourceBindingTier);
    memset(rp, 0, sizeof(rp));
    for (int i = 0; i < 2; i++) {
        rp[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rp[i].DescriptorTable.NumDescriptorRanges = 1;
        rp[i].DescriptorTable.pDescriptorRanges = &rg[i];
    }
    if (!(r2 = keep(mkrs(rp, 2)))) RET(DFL_FAIL, "root signature (u0-u63 table, t0 table)");
    if (!(p = keep(cpso(r2, SH(cs_bind), &hr)))) RET(DFL_FAIL, "CS pipeline hr=0x%08lx", (unsigned long)hr);
    if (FAILED(hr = ID3D12Device_CreateDescriptorHeap(dev, &hd, &IID_ID3D12DescriptorHeap, (void **)&big)))
        RET(DFL_FAIL, "1,000,000-descriptor shader-visible heap: hr=0x%08lx", (unsigned long)hr);
    keep(big);
    sb = keep(mkbuf(4, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE));
    ub = keep(mkbuf(8, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
    if (!sb || !ub) RET(DFL_FAIL, "buffers");
    for (UINT i = 1; i < 63; i++) uav_buf(big, 999900 + i, NULL, DXGI_FORMAT_R32_UINT, 0, 1);
    uav_buf(big, 999900, ub, DXGI_FORMAT_R32_UINT, 0, 1);
    uav_buf(big, 999963, ub, DXGI_FORMAT_R32_UINT, 1, 1);
    srv_buf(big, 999999, sb);
    begin();
    if (put_buf(sb, &src, 4, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))
        return submit(), snprintf(d, 300, "upload"), DFL_FAIL;
    barrier(ub, D3D12_RESOURCE_STATE_COMMON, UA);
    ID3D12GraphicsCommandList_SetDescriptorHeaps(list, 1, &big);
    ID3D12GraphicsCommandList_SetComputeRootSignature(list, r2);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(list, 0, gpuh(big, 999900));
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(list, 1, gpuh(big, 999999));
    ID3D12GraphicsCommandList_SetPipelineState(list, p);
    ID3D12GraphicsCommandList_Dispatch(list, 1, 1, 1);
    if (read_buf(ub, UA, 8, v)) RET(DFL_FAIL, "submit/readback failed");
    RET(v[0] == 42 && v[1] == 43 ? DFL_PASS : DFL_FAIL,
        "tier %d: t0 at index 999999 of a 1000000 heap, u0..u63 table at 999900: u0=%u u63=%u (want 42, 43)",
        opt.ResourceBindingTier, v[0], v[1]);
}

static int t_tiled(char *d)
{
    D3D12_RESOURCE_DESC rd = bufdesc(131072, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    D3D12_HEAP_DESC hd;
    D3D12_TILED_RESOURCE_COORDINATE co = { 0, 0, 0, 0 };
    D3D12_TILE_REGION_SIZE sz = { 1, FALSE, 0, 0, 0 };
    D3D12_TILE_RANGE_FLAGS rf = D3D12_TILE_RANGE_FLAG_NONE;
    UINT start = 0, count = 1;
    ID3D12Resource *r = NULL;
    ID3D12Heap *th = NULL;
    ID3D12PipelineState *p;
    HRESULT hr;
    unsigned *v, a, b;
    int e;
    if (opt.TiledResourcesTier < D3D12_TILED_RESOURCES_TIER_2)
        RET(DFL_UNSUP, "TiledResourcesTier=%d (want >= 2)", opt.TiledResourcesTier);
    if (!(p = keep(cpso(rs, SH(cs_tiled), &hr)))) RET(DFL_FAIL, "CS pipeline hr=0x%08lx", (unsigned long)hr);
    if (FAILED(hr = ID3D12Device_CreateReservedResource(dev, &rd, D3D12_RESOURCE_STATE_COMMON, NULL,
                                                        &IID_ID3D12Resource, (void **)&r)))
        RET(DFL_FAIL, "reserved 128 KiB buffer: hr=0x%08lx", (unsigned long)hr);
    keep(r);
    memset(&hd, 0, sizeof(hd));
    hd.SizeInBytes = 65536;
    hd.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    hd.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    if (FAILED(hr = ID3D12Device_CreateHeap(dev, &hd, &IID_ID3D12Heap, (void **)&th)))
        RET(DFL_FAIL, "64 KiB heap: hr=0x%08lx", (unsigned long)hr);
    keep(th);
    if (!(v = malloc(131072))) RET(DFL_FAIL, "malloc");
    /* tile 0 -> heap tile 0, tile 1 stays unmapped */
    ID3D12CommandQueue_UpdateTileMappings(queue, r, 1, &co, &sz, th, 1, &rf, &start, &count, D3D12_TILE_MAPPING_FLAG_NONE);
    begin();
    null_table();
    uav_buf(heap, 0, r, DXGI_FORMAT_R32_TYPELESS, 0, 32768);
    barrier(r, D3D12_RESOURCE_STATE_COMMON, UA);
    comp(p);
    ID3D12GraphicsCommandList_Dispatch(list, 1, 1, 1);
    e = read_buf(r, UA, 131072, v);
    a = v[0];
    b = v[16384];
    free(v);
    if (e) RET(DFL_FAIL, "submit/readback failed");
    RET(a == 0x1234 && b == 0 ? DFL_PASS : DFL_FAIL,
        "mapped tile: %#x (want 0x1234), unmapped tile: %#x (want 0: writes dropped, reads zero)", a, b);
}

/* ---- 12_1 ---- */

static int t_rov(char *d)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC g;
    D3D12_CPU_DESCRIPTOR_HANDLE h;
    ID3D12Resource *t, *rt;
    ID3D12PipelineState *p;
    HRESULT hr;
    int n = 0;
    if (!opt.ROVsSupported) RET(DFL_UNSUP, "ROVsSupported=0");
    gdesc(&g, SH(vs_inst), SH(ps_rov), RGBA8);
    if (!(p = keep(gpso(&g, &hr)))) RET(DFL_FAIL, "ROV pipeline hr=0x%08lx", (unsigned long)hr);
    t = keep(mktex(DXGI_FORMAT_R32_UINT, W, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));
    rt = mkrt(RGBA8, 1, 0, &h);
    if (!t || !rt) RET(DFL_FAIL, "resources");
    begin();
    if (put_tex(t, DXGI_FORMAT_R32_UINT, W, 4, 0, UA)) return submit(), snprintf(d, 300, "upload"), DFL_FAIL;
    null_table();
    uav_tex(1, t, DXGI_FORMAT_R32_UINT);
    clear(h, 0, 0, 0, 1);
    gfx(p, &h, 1, W, W, NULL, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_DrawInstanced(list, 3, 3, 0, 0);
    if (read_tex(t, UA, DXGI_FORMAT_R32_UINT, W, 4, px)) RET(DFL_FAIL, "submit/readback failed");
    for (int i = 0; i < W * W; i++) n += px[i] == 18;
    RET(n == W * W ? DFL_PASS : DFL_FAIL, "3 ordered read-modify-writes per pixel: %d/%d px = 18 (centre %u)", n, W * W,
        CENTER);
}

static int t_conservative(char *d)
{
    /* triangle inside pixel (10, 10) that misses its centre */
    const float c[16] = { 10.1f / 32 - 1, 1 - 10.1f / 32, 0, 0, 10.4f / 32 - 1, 1 - 10.1f / 32, 0, 0,
                          10.1f / 32 - 1, 1 - 10.4f / 32, 0, 0 };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC g;
    ID3D12Resource *cnt = keep(mkbuf(4, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
    ID3D12PipelineState *p0, *p1;
    HRESULT hr;
    unsigned off = 0, on = 0;
    if (opt.ConservativeRasterizationTier < D3D12_CONSERVATIVE_RASTERIZATION_TIER_1)
        RET(DFL_UNSUP, "ConservativeRasterizationTier=0");
    gdesc(&g, SH(vs_cb), SH(ps_count), DXGI_FORMAT_UNKNOWN);
    p0 = keep(gpso(&g, &hr));
    g.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
    if (!(p1 = keep(gpso(&g, &hr))) || !p0) RET(DFL_FAIL, "pipeline hr=0x%08lx", (unsigned long)hr);
    if (!cnt || count_draw(p0, c, 3, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, cnt, &off) ||
        count_draw(p1, c, 3, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, cnt, &on))
        RET(DFL_FAIL, "submit/readback failed");
    RET(on >= 1 && off == 0 ? DFL_PASS : DFL_FAIL,
        "sub-pixel triangle off the pixel centre: %u PS invocations conservative (want >= 1), %u normal (want 0), tier %d",
        on, off, opt.ConservativeRasterizationTier);
}

/* ---- 12_2: caps only ---- */

static int shader_model(void)
{
    for (int m = 0x68; m >= 0x51; m = m == 0x60 ? 0x51 : m - 1) {
        D3D12_FEATURE_DATA_SHADER_MODEL s = { (D3D_SHADER_MODEL)m };
        if (SUCCEEDED(ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_SHADER_MODEL, &s, sizeof(s))))
            return s.HighestShaderModel;
    }
    return 0x51;
}

static void caps_122(const char *lv, int sm)
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 o5;
    D3D12_FEATURE_DATA_D3D12_OPTIONS6 o6;
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 o7;
    memset(&o5, 0, sizeof(o5));
    memset(&o6, 0, sizeof(o6));
    memset(&o7, 0, sizeof(o7));
    ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_D3D12_OPTIONS5, &o5, sizeof(o5));
    ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_D3D12_OPTIONS6, &o6, sizeof(o6));
    ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_D3D12_OPTIONS7, &o7, sizeof(o7));
    dfl_feature(lv, "sm6_5", sm >= 0x65 ? DFL_REPORTED : DFL_UNSUP, "HighestShaderModel=%d.%d", sm >> 4, sm & 15);
    dfl_feature(lv, "mesh_shader", o7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1 ? DFL_REPORTED : DFL_UNSUP,
                "MeshShaderTier=%#x", o7.MeshShaderTier);
    dfl_feature(lv, "dxr1_1", o5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1 ? DFL_REPORTED : DFL_UNSUP,
                "RaytracingTier=%#x", o5.RaytracingTier);
    dfl_feature(lv, "vrs_tier2", o6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2 ? DFL_REPORTED
                : DFL_UNSUP, "VariableShadingRateTier=%d", o6.VariableShadingRateTier);
    dfl_feature(lv, "sampler_feedback", o7.SamplerFeedbackTier >= D3D12_SAMPLER_FEEDBACK_TIER_0_9 ? DFL_REPORTED
                : DFL_UNSUP, "SamplerFeedbackTier=%#x", o7.SamplerFeedbackTier);
}

typedef int (*test_fn)(char *d);
static const struct { int level; const char *name; test_fn fn; } TESTS[] = {
    { 0, "tess", t_tess }, { 0, "gs", t_gs }, { 0, "compute", t_compute }, { 0, "mrt", t_mrt }, { 0, "msaa4x", t_msaa },
    { 1, "uav_vs", t_uav_vs }, { 1, "logic_op", t_logic_op }, { 1, "tir", t_tir },
    { 2, "typed_uav_load", t_typed_uav }, { 2, "binding_tier2", t_binding }, { 2, "tiled_tier2", t_tiled },
    { 3, "rov", t_rov }, { 3, "conservative", t_conservative },
};

static void run(const char *lv, const char *name, test_fn fn)
{
    char d[300] = "skipped: device lost";
    int st = lost ? DFL_FAIL : fn(d);
    if (lost) st = DFL_FAIL; /* objects may still be in use: leak them */
    else while (nobj) {
        IUnknown *o = objs[--nobj];
        IUnknown_Release(o);
    }
    dfl_feature(lv, name, st, "%s", d);
}

static int init(void)
{
    D3D12_COMMAND_QUEUE_DESC qd = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
    D3D12_DESCRIPTOR_HEAP_DESC hd = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
    D3D12_DESCRIPTOR_HEAP_DESC rd = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 8, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
    D3D12_DESCRIPTOR_RANGE rg[2] = { { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0, 0 },
                                     { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 2 } };
    D3D12_ROOT_PARAMETER rp[2];
    memset(rp, 0, sizeof(rp));
    rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rp[0].Constants.Num32BitValues = 16;
    rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rp[1].DescriptorTable.NumDescriptorRanges = 2;
    rp[1].DescriptorTable.pDescriptorRanges = rg;
    if (FAILED(ID3D12Device_CreateCommandQueue(dev, &qd, &IID_ID3D12CommandQueue, (void **)&queue)) ||
        FAILED(ID3D12Device_CreateCommandAllocator(dev, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator,
                                                   (void **)&alloc)) ||
        FAILED(ID3D12Device_CreateCommandList(dev, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc, NULL,
                                              &IID_ID3D12GraphicsCommandList, (void **)&list)) ||
        FAILED(ID3D12GraphicsCommandList_Close(list)) ||
        FAILED(ID3D12Device_CreateFence(dev, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&fence)) ||
        !(fence_ev = CreateEventA(NULL, FALSE, FALSE, NULL)) ||
        FAILED(ID3D12Device_CreateDescriptorHeap(dev, &hd, &IID_ID3D12DescriptorHeap, (void **)&heap)) ||
        FAILED(ID3D12Device_CreateDescriptorHeap(dev, &rd, &IID_ID3D12DescriptorHeap, (void **)&rtvs)) ||
        !(rs = mkrs(rp, 2)))
        return 0;
    inc = ID3D12Device_GetDescriptorHandleIncrementSize(dev, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    rtv_inc = ID3D12Device_GetDescriptorHandleIncrementSize(dev, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    null_table();
    return 1;
}

/* Swapchain loop until dfl_on_present says the render time is over: a turning triangle, colour = verdict. */
static int present(HWND hw, int verdict)
{
    static const float col[4][4] = { { 0.1f, 0.8f, 0.2f, 1 }, { 0.9f, 0.1f, 0.1f, 1 }, { 0.9f, 0.6f, 0.1f, 1 },
                                     { 0.3f, 0.5f, 0.9f, 1 } };
    DXGI_SWAP_CHAIN_DESC1 sd;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC g;
    D3D12_CPU_DESCRIPTOR_HANDLE h[2];
    IDXGIFactory2 *f = NULL;
    IDXGISwapChain1 *sc1 = NULL;
    IDXGISwapChain3 *sc = NULL;
    ID3D12Resource *bb[2] = { NULL, NULL };
    ID3D12PipelineState *p;
    float c[16] = { 0 };
    HRESULT hr;
    memset(&sd, 0, sizeof(sd));
    sd.Width = 640;
    sd.Height = 360;
    sd.Format = RGBA8;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    if (FAILED(hr = CreateDXGIFactory1(&IID_IDXGIFactory2, (void **)&f))) goto fail;
    hr = IDXGIFactory2_CreateSwapChainForHwnd(f, (IUnknown *)queue, hw, &sd, NULL, NULL, &sc1);
    IDXGIFactory2_Release(f);
    if (FAILED(hr) || FAILED(hr = IDXGISwapChain1_QueryInterface(sc1, &IID_IDXGISwapChain3, (void **)&sc))) goto fail;
    IDXGISwapChain1_Release(sc1);
    for (UINT i = 0; i < 2; i++) {
        if (FAILED(hr = IDXGISwapChain3_GetBuffer(sc, i, &IID_ID3D12Resource, (void **)&bb[i]))) goto fail;
        h[i] = cpuh(rtvs, 4 + i, rtv_inc);
        ID3D12Device_CreateRenderTargetView(dev, bb[i], NULL, h[i]);
    }
    gdesc(&g, SH(vs_spin), SH(ps_vcol), RGBA8);
    if (!(p = gpso(&g, &hr))) goto fail;
    memcpy(c + 4, col[verdict], sizeof(col[0]));
    for (;;) {
        UINT i = IDXGISwapChain3_GetCurrentBackBufferIndex(sc);
        dfl_pump();
        c[0] = (float)dfl.frames * 0.03f;
        begin();
        barrier(bb[i], D3D12_RESOURCE_STATE_PRESENT, RTS);
        clear(h[i], c[4] * 0.25f, c[5] * 0.25f, c[6] * 0.25f, 1);
        gfx(p, &h[i], 1, 640, 360, c, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D12GraphicsCommandList_DrawInstanced(list, 3, 1, 0, 0);
        barrier(bb[i], RTS, D3D12_RESOURCE_STATE_PRESENT);
        exec();
        hr = IDXGISwapChain3_Present(sc, dfl.sync, 0);
        if (wait_idle() || dfl_on_present(hr)) break;
    }
    return FAILED(hr) || lost ? -1 : 0;
fail:
    printf("D3DFL: swapchain setup failed hr=0x%08lx\n", (unsigned long)hr);
    return -1;
}

int main(int argc, char **argv)
{
    char why[640], sum[200] = "";
    int want = -1, maxc = -1, v[5] = { 0 }, fail = 0, sm, rc;
    D3D12_FEATURE_DATA_FEATURE_LEVELS fls = { 5, FL, D3D_FEATURE_LEVEL_11_0 };
    HRESULT hr = E_FAIL;
    HWND hw;
    dfl_init("d3d12", argc, argv);
    for (int i = 0; i < 5; i++)
        if (!strcmp(dfl.level, LV[i])) want = i;
    if (want < 0 && strcmp(dfl.level, "all")) {
        printf("D3DFL: unknown level %s (11_0 11_1 12_0 12_1 12_2 all)\n", dfl.level);
        return dfl_finish("FAIL", "bad level");
    }
    hw = dfl_window(640, 360);
    /* Highest level first: vkd3d-proton returns an existing device for any level, so try before one exists. */
    for (int i = 4; i >= 0 && !dev; i--) {
        hr = D3D12CreateDevice(NULL, FL[i], &IID_ID3D12Device, (void **)&dev);
        if (SUCCEEDED(hr)) maxc = i;
        else {
            dev = NULL;
            printf("D3DFL: D3D12CreateDevice(%s) hr=0x%08lx\n", LV[i], (unsigned long)hr);
        }
    }
    if (!dev || !init()) {
        printf("D3DFL: level 11_0 FAIL no D3D12 device hr=0x%08lx\n", (unsigned long)hr);
        return dfl_finish("FAIL", "no D3D12 device at 11_0");
    }
    ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_FEATURE_LEVELS, &fls, sizeof(fls));
    ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_D3D12_OPTIONS, &opt, sizeof(opt));
    sm = shader_model();
    printf("D3DFL: caps created=%s reported_max=%#x sm=%d.%d binding_tier=%d tiled_tier=%d conservative_tier=%d rov=%d "
           "typed_uav_load=%d logic_op=%d\n", LV[maxc], fls.MaxSupportedFeatureLevel, sm >> 4, sm & 15,
           opt.ResourceBindingTier, opt.TiledResourcesTier, opt.ConservativeRasterizationTier, opt.ROVsSupported,
           opt.TypedUAVLoadAdditionalFormats, opt.OutputMergerLogicOp);
    for (int i = 0; i < 5; i++) {
        size_t l = strlen(sum);
        if (want >= 0 && i != want) continue;
        created = i <= maxc;
        dfl_level_begin();
        for (size_t k = 0; k < sizeof(TESTS) / sizeof(TESTS[0]); k++)
            if (TESTS[k].level == i) run(LV[i], TESTS[k].name, TESTS[k].fn);
        if (i == 4) caps_122(LV[i], sm);
        if (created) snprintf(why, sizeof(why), "device created at %s", LV[i]);
        else snprintf(why, sizeof(why), "D3D12CreateDevice(%s) failed, highest %s", LV[i], LV[maxc]);
        v[i] = dfl_level_end(LV[i], created, why);
        fail |= v[i] == DFL_FAIL;
        snprintf(sum + l, sizeof(sum) - l, "%s=%s ", LV[i], dfl_status[v[i]]);
    }
    rc = present(hw, fail ? DFL_FAIL : want >= 0 ? v[want] : DFL_PASS);
    if (rc || lost) return dfl_finish("FAIL", lost ? "device lost" : "render loop failed");
    if (want >= 0) {
        snprintf(why, sizeof(why), "%s %s", v[want] == DFL_UNSUP ? "not_created" : "created", dfl.names);
        return dfl_finish(dfl_status[v[want]], why);
    }
    snprintf(sum + strlen(sum), sizeof(sum) - strlen(sum), "highest=%s", LV[maxc]);
    return dfl_finish(fail ? "FAIL" : "DONE", sum);
}
