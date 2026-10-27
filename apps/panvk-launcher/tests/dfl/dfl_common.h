/* Shared harness of the PanPlay D3D feature-level tests (d3d12fl.c, d3d11fl.c). PanVK project code, MIT.
 * stdout (unbuffered), read by PanPlay's game loop (assets/gameloop.json tasks d3d12fl_*, d3d11fl_*):
 *   D3DFL: start api=<a> level=<l> secs=<S> sync=<N>
 *   D3DFL: feature level=<l> <name> PASS|FAIL|UNSUPPORTED|REPORTED <detail>
 *     (REPORTED = informational, not counted: a cap only, or a translation-layer limit)
 *   D3DFL: level <l> PASS|FAIL|UNSUPPORTED <detail>
 *   D3DFL: first frame startup=<s> / D3DFL: Present frame=<n> / D3DFL: fps=<f> frames=<n> t=<s>
 *   one level:  D3DFL: PASS|FAIL|UNSUPPORTED api=<a> level=<l> <detail> frames=<n> secs=<s> fps=<f> startup=<s>
 *   all levels: D3DFL: DONE|FAIL api=<a> level=all <l>=<verdict> ... frames=<n> secs=<s> fps=<f> startup=<s>
 * Exit code 0, or 1 on FAIL. Options: level = first non-option argument or D3DFL_LEVEL (default all);
 * D3DFL_SECS / -secs N: render time after the checks, from the first Present (default 20; the game loop sets it);
 * D3DFL_SYNC / -sync N: Present sync interval 0-4 (default 0). Include from one translation unit only. */
#ifndef DFL_COMMON_H
#define DFL_COMMON_H
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DFL_PASS, DFL_FAIL, DFL_UNSUP, DFL_REPORTED };
static const char *const dfl_status[] = { "PASS", "FAIL", "UNSUPPORTED", "REPORTED" };

static struct {
    const char *api, *level; /* level: one name or "all" */
    int secs, sync, done;
    unsigned frames, last_frames;
    double startup;
    LARGE_INTEGER freq, init, start, last;
    int n[4];                /* per-level feature counts by status */
    char names[512];         /* per-level "name,name" */
} dfl;

static double dfl_sec(LARGE_INTEGER a, LARGE_INTEGER b) { return (double)(b.QuadPart - a.QuadPart) / (double)dfl.freq.QuadPart; }

static void dfl_init(const char *api, int argc, char **argv)
{
    const char *e;
    setvbuf(stdout, NULL, _IONBF, 0);
    dfl.api = api;
    dfl.level = (e = getenv("D3DFL_LEVEL")) && *e ? e : "all";
    dfl.secs = (e = getenv("D3DFL_SECS")) && *e ? atoi(e) : 20;
    dfl.sync = (e = getenv("D3DFL_SYNC")) && *e ? atoi(e) : 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-secs") && i + 1 < argc) dfl.secs = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-sync") && i + 1 < argc) dfl.sync = atoi(argv[++i]);
        else if (argv[i][0] != '-') dfl.level = argv[i];
    }
    if (dfl.sync < 0 || dfl.sync > 4) dfl.sync = 0;
    QueryPerformanceFrequency(&dfl.freq);
    QueryPerformanceCounter(&dfl.init);
    printf("D3DFL: start api=%s level=%s secs=%d sync=%d\n", api, dfl.level, dfl.secs, dfl.sync);
}

static LRESULT CALLBACK dfl_wndproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    return (m == WM_CLOSE || m == WM_DESTROY) ? 0 : DefWindowProcA(h, m, w, l);
}

static HWND dfl_window(int w, int h)
{
    WNDCLASSA wc;
    RECT r = { 0, 0, w, h };
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = dfl_wndproc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "d3dfl";
    RegisterClassA(&wc);
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hw = CreateWindowA("d3dfl", "D3D feature levels", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                            r.right - r.left, r.bottom - r.top, NULL, NULL, wc.hInstance, NULL);
    if (hw) { ShowWindow(hw, SW_SHOW); UpdateWindow(hw); }
    return hw;
}

static void dfl_pump(void)
{
    MSG m;
    while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageA(&m); }
}

static void dfl_level_begin(void)
{
    memset(dfl.n, 0, sizeof(dfl.n));
    dfl.names[0] = '\0';
}

static int dfl_feature(const char *level, const char *name, int st, const char *fmt, ...)
{
    char d[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(d, sizeof(d), fmt, ap);
    va_end(ap);
    printf("D3DFL: feature level=%s %s %s %s\n", level, name, dfl_status[st], d);
    dfl.n[st]++;
    size_t l = strlen(dfl.names);
    snprintf(dfl.names + l, sizeof(dfl.names) - l, "%s%s:%s", l ? "," : "", name, dfl_status[st]);
    return st;
}

/* Level verdict: FAIL when a feature gave a wrong result (FAIL is only used for features the device claims), or when
 * the device was created at the level but a feature of it is missing; UNSUPPORTED when the device cannot be created at
 * the level; else PASS. */
static int dfl_level_end(const char *level, int created, const char *why)
{
    int v = dfl.n[DFL_FAIL] ? DFL_FAIL : !created ? DFL_UNSUP : dfl.n[DFL_UNSUP] ? DFL_FAIL : DFL_PASS;
    printf("D3DFL: level %s %s pass=%d fail=%d unsupported=%d reported=%d %s\n", level, dfl_status[v], dfl.n[DFL_PASS],
           dfl.n[DFL_FAIL], dfl.n[DFL_UNSUP], dfl.n[DFL_REPORTED], why ? why : "");
    return v;
}

/* Call after every Present. Returns 1 once the render time is over. */
static int dfl_on_present(HRESULT hr)
{
    LARGE_INTEGER t;
    double dt, el;
    if (dfl.done) return 1;
    QueryPerformanceCounter(&t);
    if (!dfl.frames) {
        dfl.startup = dfl_sec(dfl.init, t);
        dfl.start = dfl.last = t;
        dfl.last_frames = 1;
        printf("D3DFL: first frame startup=%.1f\n", dfl.startup);
    }
    dfl.frames++;
    if (dfl.frames <= 10 || dfl.frames % 60 == 0)
        printf("D3DFL: Present frame=%u hr=0x%08lx\n", dfl.frames, (unsigned long)hr);
    dt = dfl_sec(dfl.last, t);
    el = dfl_sec(dfl.start, t);
    if (dt >= 1.0) {
        printf("D3DFL: fps=%.1f frames=%u t=%.1f\n", (dfl.frames - dfl.last_frames) / dt, dfl.frames, el);
        dfl.last_frames = dfl.frames;
        dfl.last = t;
    }
    if (FAILED(hr) || el >= dfl.secs) dfl.done = 1;
    return dfl.done;
}

/* Final line; returns the exit code. */
static int dfl_finish(const char *verdict, const char *detail)
{
    LARGE_INTEGER t;
    double el;
    QueryPerformanceCounter(&t);
    el = dfl.frames ? dfl_sec(dfl.start, t) : 0.0;
    printf("D3DFL: %s api=%s level=%s %s frames=%u secs=%.1f fps=%.1f startup=%.1f\n", verdict, dfl.api, dfl.level, detail,
           dfl.frames, el, el > 0 ? (dfl.frames - 1) / el : 0.0, dfl.startup);
    fflush(stdout);
    return !strcmp(verdict, "FAIL");
}
#endif
