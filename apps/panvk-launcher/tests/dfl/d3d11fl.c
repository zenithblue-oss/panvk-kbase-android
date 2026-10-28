/* PanPlay D3D11 feature-level test (DXVK). For each level it creates a device at exactly that level, draws or
 * dispatches the level's defining features with shaders of the level's model, checks them by readback, then renders
 * for -secs N seconds.
 *   10_0 (SM 4.0): geometry shader, stream output
 *   10_1 (SM 4.1): cube map array, Gather
 *   11_0 (SM 5.0): tessellation, compute shader UAV
 *   11_1: UAV in the vertex shader, logic ops, target-independent rasterization (ForcedSampleCount)
 * A level the device cannot be created at is UNSUPPORTED (its features are not run).
 * Usage: d3d11fl.exe [10_0|10_1|11_0|11_1|all] [-secs N] [-sync N]. Output: dfl_common.h.
 * Shaders: dfl.hlsl / dfl4.hlsl, precompiled into dfl_shaders.h by build-dfl.sh. PanVK project code, MIT. */
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#include <initguid.h>
#include <d3d11_3.h>
#include "dfl_common.h"
#include "dfl_shaders.h"

#define W 64
#define RGBA8 DXGI_FORMAT_R8G8B8A8_UNORM
#define SHB(n) n##_dxbc, sizeof(n##_dxbc)
#define RET(st, ...) do { snprintf(d, 300, __VA_ARGS__); return (st); } while (0)
#define PR(v) ((int)((v) & 0xff))
#define PG(v) ((int)(((v) >> 8) & 0xff))
#define PB(v) ((int)(((v) >> 16) & 0xff))
#define PA(v) ((int)((v) >> 24))
#define CENTER px[W / 2 * W + W / 2]

static const char *const LV[] = { "10_0", "10_1", "11_0", "11_1" };
static const D3D_FEATURE_LEVEL FL[] = { D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0,
                                        D3D_FEATURE_LEVEL_11_1 };

static ID3D11Device *dev;
static ID3D11DeviceContext *ctx;
static ID3D11Buffer *cb;           /* b0: float4 p[4] */
static ID3D11RasterizerState *rs;  /* no culling */
static ID3D11SamplerState *smp;    /* point, clamp */
static IUnknown *objs[24];         /* per-test objects */
static int nobj;
static const char *cur_lv;
static unsigned px[W * W];

static void *keep(void *p)
{
    if (p && nobj < 24) objs[nobj++] = p;
    return p;
}

static ID3D11Texture2D *mktex(DXGI_FORMAT fmt, UINT w, UINT h, UINT bind, const void *data, UINT row)
{
    D3D11_TEXTURE2D_DESC t = { w, h, 1, 1, fmt, { 1, 0 }, D3D11_USAGE_DEFAULT, bind, 0, 0 };
    D3D11_SUBRESOURCE_DATA s = { data, row, 0 };
    ID3D11Texture2D *r = NULL;
    ID3D11Device_CreateTexture2D(dev, &t, data ? &s : NULL, &r);
    return keep(r);
}

static ID3D11RenderTargetView *mkrt(DXGI_FORMAT fmt, ID3D11Texture2D **t)
{
    ID3D11RenderTargetView *v = NULL;
    if ((*t = mktex(fmt, W, W, D3D11_BIND_RENDER_TARGET, NULL, 0)))
        ID3D11Device_CreateRenderTargetView(dev, (ID3D11Resource *)*t, NULL, &v);
    return keep(v);
}

static ID3D11Buffer *mkbuf(UINT size, UINT bind, UINT misc, const void *data)
{
    D3D11_BUFFER_DESC b = { size, D3D11_USAGE_DEFAULT, bind, 0, misc, misc ? 4 : 0 };
    D3D11_SUBRESOURCE_DATA s = { data, 0, 0 };
    ID3D11Buffer *r = NULL;
    ID3D11Device_CreateBuffer(dev, &b, data ? &s : NULL, &r);
    return r;
}

/* structured uint buffer with a UAV, zero-filled */
static ID3D11UnorderedAccessView *mkuav(UINT n, ID3D11Buffer **b)
{
    static const unsigned zero[64];
    D3D11_UNORDERED_ACCESS_VIEW_DESC u;
    ID3D11UnorderedAccessView *v = NULL;
    memset(&u, 0, sizeof(u));
    u.Format = DXGI_FORMAT_UNKNOWN;
    u.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    u.Buffer.NumElements = n;
    if ((*b = keep(mkbuf(n * 4, D3D11_BIND_UNORDERED_ACCESS, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, zero))))
        ID3D11Device_CreateUnorderedAccessView(dev, (ID3D11Resource *)*b, &u, &v);
    return keep(v);
}

/* Staging copy of t (W x W or smaller) into out, tight rows. */
static int read_tex(ID3D11Texture2D *t, UINT bpp, void *out)
{
    D3D11_TEXTURE2D_DESC td;
    D3D11_MAPPED_SUBRESOURCE m;
    ID3D11Texture2D *s = NULL;
    int r = -1;
    ID3D11Texture2D_GetDesc(t, &td);
    td.Usage = D3D11_USAGE_STAGING;
    td.BindFlags = 0;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    td.MiscFlags = 0;
    if (FAILED(ID3D11Device_CreateTexture2D(dev, &td, NULL, &s))) return -1;
    ID3D11DeviceContext_CopyResource(ctx, (ID3D11Resource *)s, (ID3D11Resource *)t);
    if (SUCCEEDED(ID3D11DeviceContext_Map(ctx, (ID3D11Resource *)s, 0, D3D11_MAP_READ, 0, &m))) {
        for (UINT y = 0; y < td.Height; y++)
            memcpy((char *)out + y * td.Width * bpp, (char *)m.pData + y * m.RowPitch, td.Width * bpp);
        ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource *)s, 0);
        r = 0;
    }
    ID3D11Texture2D_Release(s);
    return r;
}

static int read_buf(ID3D11Buffer *b, void *out, UINT size)
{
    D3D11_BUFFER_DESC bd;
    D3D11_MAPPED_SUBRESOURCE m;
    ID3D11Buffer *s = NULL;
    int r = -1;
    ID3D11Buffer_GetDesc(b, &bd);
    bd.Usage = D3D11_USAGE_STAGING;
    bd.BindFlags = 0;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    bd.MiscFlags = bd.StructureByteStride = 0; /* plain staging copy, as in the BasicCompute11 sample */
    if (FAILED(ID3D11Device_CreateBuffer(dev, &bd, NULL, &s))) return -1;
    ID3D11DeviceContext_CopyResource(ctx, (ID3D11Resource *)s, (ID3D11Resource *)b);
    if (SUCCEEDED(ID3D11DeviceContext_Map(ctx, (ID3D11Resource *)s, 0, D3D11_MAP_READ, 0, &m))) {
        memcpy(out, m.pData, size);
        ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource *)s, 0);
        r = 0;
    }
    ID3D11Buffer_Release(s);
    return r;
}

static ID3D11VertexShader *vs(const void *b, SIZE_T n)
{
    ID3D11VertexShader *s = NULL;
    ID3D11Device_CreateVertexShader(dev, b, n, NULL, &s);
    return keep(s);
}

static ID3D11PixelShader *ps(const void *b, SIZE_T n)
{
    ID3D11PixelShader *s = NULL;
    ID3D11Device_CreatePixelShader(dev, b, n, NULL, &s);
    return keep(s);
}

/* Common draw state: constants c, viewport w x h, no culling, shaders v/p, no GS/HS/DS. */
static void draw_state(const float *c, UINT w, UINT h, ID3D11VertexShader *v, ID3D11PixelShader *p,
                       D3D11_PRIMITIVE_TOPOLOGY topo)
{
    static const float zero16[16];
    D3D11_VIEWPORT vp = { 0, 0, (float)w, (float)h, 0, 1 };
    ID3D11DeviceContext_ClearState(ctx);
    ID3D11DeviceContext_UpdateSubresource(ctx, (ID3D11Resource *)cb, 0, NULL, c ? c : zero16, 0, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(ctx, 0, 1, &cb);
    ID3D11DeviceContext_GSSetConstantBuffers(ctx, 0, 1, &cb);
    ID3D11DeviceContext_PSSetConstantBuffers(ctx, 0, 1, &cb);
    ID3D11DeviceContext_RSSetState(ctx, rs);
    ID3D11DeviceContext_RSSetViewports(ctx, 1, &vp);
    ID3D11DeviceContext_IASetPrimitiveTopology(ctx, topo);
    ID3D11DeviceContext_VSSetShader(ctx, v, NULL, 0);
    ID3D11DeviceContext_PSSetShader(ctx, p, NULL, 0);
}

static void clear(ID3D11RenderTargetView *rt, float r, float g, float b, float a)
{
    const float c[4] = { r, g, b, a };
    ID3D11DeviceContext_ClearRenderTargetView(ctx, rt, c);
}

static int count_green(void)
{
    int n = 0;
    for (int i = 0; i < W * W; i++) n += PG(px[i]) > 200 && PR(px[i]) < 60;
    return n;
}

/* ---- 10_0 (shader model 4.0) ---- */

/* One point in, a full-screen strip out of the GS, rasterized and streamed out; reports "gs", returns stream_out. */
static int t_gs_so(char *d)
{
    static const D3D11_SO_DECLARATION_ENTRY so = { 0, "SV_Position", 0, 0, 4, 0 };
    static const float want[12] = { -1, -1, 0, 1, -1, 1, 0, 1, 1, -1, 0, 1 };
    const float c[16] = { -0.95f, -0.95f };
    UINT stride = 16, off = 0;
    ID3D11Texture2D *t;
    ID3D11RenderTargetView *rt = mkrt(RGBA8, &t);
    ID3D11VertexShader *v = vs(SHB(vs_cb_4));
    ID3D11PixelShader *p = ps(SHB(ps_green_4));
    ID3D11GeometryShader *g = NULL;
    ID3D11Buffer *sob = keep(mkbuf(256, D3D11_BIND_STREAM_OUTPUT, 0, NULL)), *none = NULL;
    HRESULT hr = ID3D11Device_CreateGeometryShaderWithStreamOutput(dev, SHB(gs_quad_4), &so, 1, &stride, 1, 0, NULL, &g);
    float out[12];
    int n, bad = 0;
    keep(g);
    if (FAILED(hr) || !g) {
        dfl_feature(cur_lv, "gs", DFL_FAIL, "GS with stream output: hr=0x%08lx", (unsigned long)hr);
        RET(DFL_FAIL, "GS with stream output: hr=0x%08lx", (unsigned long)hr);
    }
    if (!rt || !v || !p || !sob) RET(DFL_FAIL, "resources");
    draw_state(c, W, W, v, p, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    ID3D11DeviceContext_GSSetShader(ctx, g, NULL, 0);
    ID3D11DeviceContext_SOSetTargets(ctx, 1, &sob, &off);
    ID3D11DeviceContext_OMSetRenderTargets(ctx, 1, &rt, NULL);
    clear(rt, 0, 0, 0, 1);
    ID3D11DeviceContext_Draw(ctx, 1, 0);
    ID3D11DeviceContext_SOSetTargets(ctx, 1, &none, &off);
    if (read_tex(t, 4, px) || read_buf(sob, out, sizeof(out))) RET(DFL_FAIL, "readback failed");
    n = count_green();
    dfl_feature(cur_lv, "gs", n >= W * W * 9 / 10 ? DFL_PASS : DFL_FAIL,
                "one point in, full-screen strip out: %d/%d px covered", n, W * W);
    for (int i = 0; i < 12; i++) bad += out[i] != want[i];
    RET(!bad ? DFL_PASS : DFL_FAIL, "first triangle of the streamed strip: (%g,%g) (%g,%g) (%g,%g), %d/12 values wrong",
        out[0], out[1], out[4], out[5], out[8], out[9], bad);
}

/* ---- 10_1 (shader model 4.1) ---- */

static int t_cube_array(char *d)
{
    D3D11_TEXTURE2D_DESC td = { 1, 1, 1, 12, RGBA8, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0,
                                D3D11_RESOURCE_MISC_TEXTURECUBE };
    D3D11_SUBRESOURCE_DATA s[12];
    D3D11_SHADER_RESOURCE_VIEW_DESC sv;
    unsigned texel[12];
    ID3D11Texture2D *tex = NULL, *t;
    ID3D11ShaderResourceView *srv = NULL;
    ID3D11RenderTargetView *rt = mkrt(RGBA8, &t);
    ID3D11VertexShader *v = vs(SHB(vs_full_4));
    ID3D11PixelShader *p = ps(SHB(ps_cubearr_41));
    HRESULT hr;
    for (int i = 0; i < 12; i++) {
        texel[i] = i == 6 ? 0xff00ff00u : 0xff0000ffu; /* +X face of cube 1 green, the rest red */
        s[i].pSysMem = &texel[i];
        s[i].SysMemPitch = 4;
        s[i].SysMemSlicePitch = 0;
    }
    if (FAILED(hr = ID3D11Device_CreateTexture2D(dev, &td, s, &tex))) RET(DFL_FAIL, "2-cube array: hr=0x%08lx", (unsigned long)hr);
    keep(tex);
    memset(&sv, 0, sizeof(sv));
    sv.Format = RGBA8;
    sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
    sv.TextureCubeArray.MipLevels = 1;
    sv.TextureCubeArray.NumCubes = 2;
    if (FAILED(hr = ID3D11Device_CreateShaderResourceView(dev, (ID3D11Resource *)tex, &sv, &srv)))
        RET(DFL_FAIL, "TEXTURECUBEARRAY view: hr=0x%08lx", (unsigned long)hr);
    keep(srv);
    if (!rt || !v || !p) RET(DFL_FAIL, "resources or ps_4_1 shader");
    draw_state(NULL, W, W, v, p, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_PSSetShaderResources(ctx, 0, 1, &srv);
    ID3D11DeviceContext_PSSetSamplers(ctx, 0, 1, &smp);
    ID3D11DeviceContext_OMSetRenderTargets(ctx, 1, &rt, NULL);
    clear(rt, 0, 0, 0, 1);
    ID3D11DeviceContext_Draw(ctx, 3, 0);
    if (read_tex(t, 4, px)) RET(DFL_FAIL, "readback failed");
    RET(CENTER == 0xff00ff00u ? DFL_PASS : DFL_FAIL, "cube 1, +X face: %08x (want ff00ff00)", CENTER);
}

static int t_gather(char *d)
{
    static const unsigned char texel[4] = { 10, 20, 30, 40 }; /* 2x2 R8: rows (10 20) (30 40) */
    ID3D11Texture2D *tex = mktex(DXGI_FORMAT_R8_UNORM, 2, 2, D3D11_BIND_SHADER_RESOURCE, texel, 2), *t;
    ID3D11ShaderResourceView *srv = NULL;
    ID3D11RenderTargetView *rt = mkrt(RGBA8, &t);
    ID3D11VertexShader *v = vs(SHB(vs_full_4));
    ID3D11PixelShader *p = ps(SHB(ps_gather_41));
    unsigned c;
    if (!tex || FAILED(ID3D11Device_CreateShaderResourceView(dev, (ID3D11Resource *)tex, NULL, &srv)))
        RET(DFL_FAIL, "R8 texture");
    keep(srv);
    if (!rt || !v || !p) RET(DFL_FAIL, "resources or ps_4_1 shader");
    draw_state(NULL, W, W, v, p, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_PSSetShaderResources(ctx, 0, 1, &srv);
    ID3D11DeviceContext_PSSetSamplers(ctx, 0, 1, &smp);
    ID3D11DeviceContext_OMSetRenderTargets(ctx, 1, &rt, NULL);
    clear(rt, 0, 0, 0, 1);
    ID3D11DeviceContext_Draw(ctx, 3, 0);
    if (read_tex(t, 4, px)) RET(DFL_FAIL, "readback failed");
    c = CENTER; /* Gather4 order: x = (0,1), y = (1,1), z = (1,0), w = (0,0) */
    RET(abs(PR(c) - 30) <= 1 && abs(PG(c) - 40) <= 1 && abs(PB(c) - 20) <= 1 && abs(PA(c) - 10) <= 1 ? DFL_PASS : DFL_FAIL,
        "Gather of a 2x2 R8 texture: (%d,%d,%d,%d) (want 30,40,20,10)", PR(c), PG(c), PB(c), PA(c));
}

/* ---- 11_0 (shader model 5.0) ---- */

static int t_tess(char *d)
{
    const float c[16] = { -0.9f, 0, 0, 0, 0.9f, 0, 0, 0, 0, 0, 0, 0 }; /* zero-area patch on y = 0 */
    ID3D11Texture2D *t;
    ID3D11RenderTargetView *rt = mkrt(RGBA8, &t);
    ID3D11VertexShader *v = vs(SHB(vs_ctl));
    ID3D11PixelShader *p = ps(SHB(ps_green));
    ID3D11HullShader *h = NULL;
    ID3D11DomainShader *ds = NULL;
    int n;
    ID3D11Device_CreateHullShader(dev, SHB(hs_main), NULL, &h);
    ID3D11Device_CreateDomainShader(dev, SHB(ds_bulge), NULL, &ds);
    keep(h);
    keep(ds);
    if (!rt || !v || !p || !h || !ds) RET(DFL_FAIL, "resources or HS/DS");
    draw_state(c, W, W, v, p, D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    ID3D11DeviceContext_HSSetShader(ctx, h, NULL, 0);
    ID3D11DeviceContext_DSSetShader(ctx, ds, NULL, 0);
    ID3D11DeviceContext_OMSetRenderTargets(ctx, 1, &rt, NULL);
    clear(rt, 0, 0, 0, 1);
    ID3D11DeviceContext_Draw(ctx, 3, 0);
    if (read_tex(t, 4, px)) RET(DFL_FAIL, "readback failed");
    n = count_green();
    RET(n >= 20 ? DFL_PASS : DFL_FAIL, "zero-area tri patch, factor 4, interior lifted by the DS: %d px covered (want >= 20)", n);
}

static int t_compute(char *d)
{
    ID3D11Buffer *b;
    ID3D11UnorderedAccessView *u = mkuav(64, &b), *none = NULL;
    ID3D11ComputeShader *cs = NULL;
    unsigned v[64];
    int bad = 0;
    ID3D11Device_CreateComputeShader(dev, SHB(cs_write), NULL, &cs);
    keep(cs);
    if (!u || !cs) RET(DFL_FAIL, "UAV or CS");
    ID3D11DeviceContext_ClearState(ctx);
    ID3D11DeviceContext_CSSetShader(ctx, cs, NULL, 0);
    ID3D11DeviceContext_CSSetUnorderedAccessViews(ctx, 0, 1, &u, NULL);
    ID3D11DeviceContext_Dispatch(ctx, 1, 1, 1);
    ID3D11DeviceContext_CSSetUnorderedAccessViews(ctx, 0, 1, &none, NULL);
    if (read_buf(b, v, sizeof(v))) RET(DFL_FAIL, "readback failed");
    for (unsigned i = 0; i < 64; i++) bad += v[i] != i * 3 + 1;
    RET(!bad ? DFL_PASS : DFL_FAIL, "64 threads wrote i*3+1 to a structured UAV: %d wrong (v[63]=%u)", bad, v[63]);
}

/* ---- 11_1 ---- */

static int t_uav_vs(char *d)
{
    ID3D11Buffer *b;
    ID3D11UnorderedAccessView *u = mkuav(1, &b);
    ID3D11Texture2D *t;
    ID3D11RenderTargetView *rt = mkrt(RGBA8, &t);
    ID3D11VertexShader *v = vs(SHB(vs_uav));
    ID3D11PixelShader *p = ps(SHB(ps_green));
    unsigned n = 0;
    if (!u || !rt || !v || !p) RET(DFL_FAIL, "resources or shaders");
    draw_state(NULL, W, W, v, p, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews(ctx, 1, &rt, NULL, 1, 1, &u, NULL);
    clear(rt, 0, 0, 0, 1);
    ID3D11DeviceContext_Draw(ctx, 3, 0);
    ID3D11DeviceContext_ClearState(ctx);
    if (read_buf(b, &n, 4)) RET(DFL_FAIL, "readback failed");
    RET(n >= 3 ? DFL_PASS : DFL_FAIL, "VS atomics on u1 (OM UAV slot 1): %u (want >= 3)", n);
}

/* Plain draw of (ff,0f,f0,aa), then a logic-op INVERT draw: ~ = (00,f0,0f,55). Independent of integer clears. */
static int t_logic_op(char *d)
{
    D3D11_FEATURE_DATA_D3D11_OPTIONS o;
    D3D11_BLEND_DESC1 bd;
    ID3D11Device1 *dev1 = NULL;
    ID3D11BlendState1 *bs = NULL;
    ID3D11Texture2D *t;
    ID3D11RenderTargetView *rt;
    ID3D11VertexShader *v;
    ID3D11PixelShader *p;
    HRESULT hr;
    memset(&o, 0, sizeof(o));
    ID3D11Device_CheckFeatureSupport(dev, D3D11_FEATURE_D3D11_OPTIONS, &o, sizeof(o));
    if (!o.OutputMergerLogicOp) RET(DFL_UNSUP, "OutputMergerLogicOp=0");
    if (FAILED(ID3D11Device_QueryInterface(dev, &IID_ID3D11Device1, (void **)&dev1))) RET(DFL_FAIL, "no ID3D11Device1");
    keep(dev1);
    memset(&bd, 0, sizeof(bd));
    bd.RenderTarget[0].LogicOpEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlend = bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOp = bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].LogicOp = D3D11_LOGIC_OP_INVERT;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(hr = ID3D11Device1_CreateBlendState1(dev1, &bd, &bs))) RET(DFL_FAIL, "CreateBlendState1 hr=0x%08lx", (unsigned long)hr);
    keep(bs);
    rt = mkrt(DXGI_FORMAT_R8G8B8A8_UINT, &t);
    v = vs(SHB(vs_full));
    p = ps(SHB(ps_uint));
    if (!rt || !v || !p) RET(DFL_FAIL, "resources or shaders");
    draw_state(NULL, W, W, v, p, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_OMSetRenderTargets(ctx, 1, &rt, NULL);
    clear(rt, 0, 0, 0, 0);
    ID3D11DeviceContext_Draw(ctx, 3, 0);
    ID3D11DeviceContext_OMSetBlendState(ctx, (ID3D11BlendState *)bs, NULL, ~0u);
    ID3D11DeviceContext_Draw(ctx, 3, 0);
    ID3D11DeviceContext_OMSetBlendState(ctx, NULL, NULL, ~0u);
    if (read_tex(t, 4, px)) RET(DFL_FAIL, "readback failed");
    RET(CENTER == 0x550ff000u ? DFL_PASS : DFL_FAIL, "INVERT after writing aaf00fff: %08x (want 550ff000)", CENTER);
}

/* A band y in [10.05, 10.45] px covers the 4x sample rows 10.125 and 10.375 but no pixel centre: only with the
 * forced sample count (target-independent rasterization, UAV-only rendering) does the pixel shader run.
 * The forced count is set through D3D11_RASTERIZER_DESC2 (ID3D11Device3::CreateRasterizerState2): DXVK 3.1.1
 * promotes a D3D11_RASTERIZER_DESC1 with ForcedSampleCount = 0 (D3D11RasterizerState::PromoteDesc), so the DESC1
 * state never reaches Vulkan with 4 samples on any GPU. Its count is reported, not checked. */
static int t_tir(char *d)
{
    const float y0 = 1 - 10.05f / 32, y1 = 1 - 10.45f / 32;
    const float c[16] = { -1, y0, 0, 0, 1, y0, 0, 0, -1, y1, 0, 0, 1, y1, 0, 0 };
    D3D11_RASTERIZER_DESC2 rd;
    ID3D11Device1 *dev1 = NULL;
    ID3D11Device3 *dev3 = NULL;
    ID3D11RasterizerState *r[3] = { NULL, NULL, NULL };
    ID3D11Buffer *b;
    ID3D11UnorderedAccessView *u = mkuav(1, &b);
    ID3D11VertexShader *v = vs(SHB(vs_cb));
    ID3D11PixelShader *p = ps(SHB(ps_count));
    unsigned n[3] = { 0, 0, 0 };
    if (FAILED(ID3D11Device_QueryInterface(dev, &IID_ID3D11Device1, (void **)&dev1))) RET(DFL_FAIL, "no ID3D11Device1");
    keep(dev1);
    if (FAILED(ID3D11Device_QueryInterface(dev, &IID_ID3D11Device3, (void **)&dev3))) dev3 = NULL;
    else keep(dev3);
    memset(&rd, 0, sizeof(rd));
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    /* r[0]: no forced count, r[1]: 4 through DESC1, r[2]: 4 through DESC2. */
    for (int i = 0; i < 2; i++) {
        rd.ForcedSampleCount = i ? 4 : 0;
        ID3D11Device1_CreateRasterizerState1(dev1, (const D3D11_RASTERIZER_DESC1 *)&rd,
                                             (ID3D11RasterizerState1 **)&r[i]);
        keep(r[i]);
    }
    if (dev3) {
        ID3D11Device3_CreateRasterizerState2(dev3, &rd, (ID3D11RasterizerState2 **)&r[2]);
        keep(r[2]);
    }
    if (!r[0] || !r[1] || !u || !v || !p) RET(DFL_FAIL, "ForcedSampleCount=4 rasterizer state or resources");
    for (int i = 0; i < 3 && r[i]; i++) {
        static const unsigned zero[4];
        draw_state(c, W, W, v, p, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        ID3D11DeviceContext_RSSetState(ctx, (ID3D11RasterizerState *)r[i]);
        ID3D11DeviceContext_ClearUnorderedAccessViewUint(ctx, u, zero);
        ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews(ctx, 0, NULL, NULL, 1, 1, &u, NULL);
        ID3D11DeviceContext_Draw(ctx, 4, 0);
        ID3D11DeviceContext_ClearState(ctx);
        if (read_buf(b, &n[i], 4)) RET(DFL_FAIL, "readback failed");
    }
    if (!r[2]) RET(DFL_FAIL, "no ID3D11Device3::CreateRasterizerState2 (DESC1 ForcedSampleCount=4: %u, 0: %u)", n[1], n[0]);
    RET(n[2] >= 32 && n[2] > n[0] ? DFL_PASS : DFL_FAIL,
        "UAV-only band between pixel centres: %u PS invocations with ForcedSampleCount=4 (DESC2, want >= 32), %u with 0; "
        "DESC1 ForcedSampleCount=4: %u (DXVK drops it)", n[2], n[0], n[1]);
}

typedef int (*test_fn)(char *d);
static const struct { int level; const char *name; test_fn fn; } TESTS[] = {
    { 0, "stream_out", t_gs_so },
    { 1, "cube_array", t_cube_array }, { 1, "gather4", t_gather },
    { 2, "tess", t_tess }, { 2, "compute", t_compute },
    { 3, "uav_vs", t_uav_vs }, { 3, "logic_op", t_logic_op }, { 3, "tir", t_tir },
};

static void run(const char *lv, const char *name, test_fn fn)
{
    char d[300] = "";
    int st;
    cur_lv = lv;
    st = fn(d);
    ID3D11DeviceContext_ClearState(ctx);
    while (nobj) {
        IUnknown *o = objs[--nobj];
        IUnknown_Release(o);
    }
    dfl_feature(lv, name, st, "%s", d);
}

static void freedev(void)
{
    if (smp) ID3D11SamplerState_Release(smp);
    if (rs) ID3D11RasterizerState_Release(rs);
    if (cb) ID3D11Buffer_Release(cb);
    if (ctx) ID3D11DeviceContext_Release(ctx);
    if (dev) ID3D11Device_Release(dev);
    smp = NULL;
    rs = NULL;
    cb = NULL;
    ctx = NULL;
    dev = NULL;
}

/* Shared state of a new device: constant buffer, no-cull rasterizer, point sampler. */
static int setup(void)
{
    D3D11_RASTERIZER_DESC rd;
    D3D11_SAMPLER_DESC sd;
    memset(&rd, 0, sizeof(rd));
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    memset(&sd, 0, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    return (cb = mkbuf(64, D3D11_BIND_CONSTANT_BUFFER, 0, NULL)) &&
           SUCCEEDED(ID3D11Device_CreateRasterizerState(dev, &rd, &rs)) &&
           SUCCEEDED(ID3D11Device_CreateSamplerState(dev, &sd, &smp));
}

static int present(HWND hw, int verdict)
{
    static const float col[4][4] = { { 0.1f, 0.8f, 0.2f, 1 }, { 0.9f, 0.1f, 0.1f, 1 }, { 0.9f, 0.6f, 0.1f, 1 },
                                     { 0.3f, 0.5f, 0.9f, 1 } };
    static const D3D_FEATURE_LEVEL lv[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                            D3D_FEATURE_LEVEL_10_0 };
    DXGI_SWAP_CHAIN_DESC sd;
    IDXGISwapChain *sc = NULL;
    ID3D11Texture2D *bb = NULL;
    ID3D11RenderTargetView *rt = NULL;
    ID3D11VertexShader *v;
    ID3D11PixelShader *p;
    float c[16] = { 0 };
    HRESULT hr;
    memset(&sd, 0, sizeof(sd));
    sd.BufferDesc.Width = 640;
    sd.BufferDesc.Height = 360;
    sd.BufferDesc.Format = RGBA8;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = hw;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, lv, 4, D3D11_SDK_VERSION, &sd, &sc, &dev,
                                       NULL, &ctx);
    if (FAILED(hr) || !setup() || FAILED(hr = IDXGISwapChain_GetBuffer(sc, 0, &IID_ID3D11Texture2D, (void **)&bb)) ||
        FAILED(hr = ID3D11Device_CreateRenderTargetView(dev, (ID3D11Resource *)bb, NULL, &rt))) {
        printf("D3DFL: swapchain setup failed hr=0x%08lx\n", (unsigned long)hr);
        return -1;
    }
    v = vs(SHB(vs_spin_4));
    p = ps(SHB(ps_vcol_4));
    memcpy(c + 4, col[verdict], sizeof(col[0]));
    for (;;) {
        dfl_pump();
        c[0] = (float)dfl.frames * 0.03f;
        draw_state(c, 640, 360, v, p, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D11DeviceContext_OMSetRenderTargets(ctx, 1, &rt, NULL);
        clear(rt, c[4] * 0.25f, c[5] * 0.25f, c[6] * 0.25f, 1);
        ID3D11DeviceContext_Draw(ctx, 3, 0);
        hr = IDXGISwapChain_Present(sc, dfl.sync, 0);
        if (dfl_on_present(hr)) break;
    }
    return FAILED(hr) ? -1 : 0;
}

int main(int argc, char **argv)
{
    char why[640], sum[200] = "";
    int want = -1, v[4] = { 0 }, fail = 0, rc;
    HWND hw;
    dfl_init("d3d11", argc, argv);
    for (int i = 0; i < 4; i++)
        if (!strcmp(dfl.level, LV[i])) want = i;
    if (want < 0 && strcmp(dfl.level, "all")) {
        printf("D3DFL: unknown level %s (10_0 10_1 11_0 11_1 all)\n", dfl.level);
        return dfl_finish("FAIL", "bad level");
    }
    hw = dfl_window(640, 360);
    for (int i = 0; i < 4; i++) {
        D3D_FEATURE_LEVEL got = 0;
        HRESULT hr;
        size_t l = strlen(sum);
        int created;
        if (want >= 0 && i != want) continue;
        dfl_level_begin();
        hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &FL[i], 1, D3D11_SDK_VERSION, &dev, &got, &ctx);
        created = SUCCEEDED(hr) && got == FL[i] && setup();
        if (created) {
            for (size_t k = 0; k < sizeof(TESTS) / sizeof(TESTS[0]); k++)
                if (TESTS[k].level == i) run(LV[i], TESTS[k].name, TESTS[k].fn);
            snprintf(why, sizeof(why), "device created at %s", LV[i]);
        } else
            snprintf(why, sizeof(why), "D3D11CreateDevice(%s) hr=0x%08lx got=%#x", LV[i], (unsigned long)hr, got);
        freedev();
        v[i] = dfl_level_end(LV[i], created, why);
        fail |= v[i] == DFL_FAIL;
        snprintf(sum + l, sizeof(sum) - l, "%s=%s ", LV[i], dfl_status[v[i]]);
    }
    rc = present(hw, fail ? DFL_FAIL : want >= 0 ? v[want] : DFL_PASS);
    if (rc) return dfl_finish("FAIL", "render loop failed");
    if (want >= 0) {
        snprintf(why, sizeof(why), "%s %s", v[want] == DFL_UNSUP ? "not_created" : "created", dfl.names);
        return dfl_finish(dfl_status[v[want]], why);
    }
    if (*sum) sum[strlen(sum) - 1] = '\0';
    return dfl_finish(fail ? "FAIL" : "DONE", sum);
}
