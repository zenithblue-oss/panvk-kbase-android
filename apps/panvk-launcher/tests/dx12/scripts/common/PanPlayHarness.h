/* PanPlay test harness hooks for the D3D12 test builds (PanVK project code, MIT). Works in C99 and C++.
 * stdout lines, in the same style as the tests/dxcube.c "CUBE:" lines:
 *   DX12: start test=<name> secs=<S> sync=<N> <W>x<H>
 *   DX12: first frame startup=<s>          (seconds from process start to the first Present)
 *   DX12: Present frame=<n> hr=0x...      (frames 1-10, then every 60th)
 *   DX12: fps=<f> frames=<n> t=<s>        (about once per second; t = seconds since the first frame)
 *   DX12: PASS test=<name> frames=<n> secs=<s> fps=<f> startup=<s>   (the app then quits with exit code 0)
 *   DX12: FAIL <what> hr=0x...             (exit code 1)
 * Options: env DX12TEST_SECS (default 30; 0 = run until the window is closed) and
 * DX12TEST_SYNC (Present sync interval 0-4, default 0), or the args -secs N -sync N.
 * The run time and the fps average count from the first Present, as DXCUBE_SECS starts after setup, so
 * asset loading and pipeline compilation do not skew them. startup= reports that part on its own.
 * In C++ the state is a single program-wide object (a static inside an inline function). In C, include
 * this header from one translation unit only. */
#ifndef PANPLAY_HARNESS_H
#define PANPLAY_HARNESS_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#ifdef __cplusplus
#define PPH_FN inline
#else
#define PPH_FN static inline
#endif

struct pph_state
{
    const char *test;
    int secs, sync, done;
    unsigned frames, last_frames;
    double startup;
    LARGE_INTEGER freq, init, start, last;
};

PPH_FN struct pph_state *pph(void)
{
    static struct pph_state s = { "d3d12", 30, 0, 0, 0, 0, 0.0, {{0}}, {{0}}, {{0}}, {{0}} };
    return &s;
}

PPH_FN double pph_seconds(LARGE_INTEGER from, LARGE_INTEGER to)
{
    return (double)(to.QuadPart - from.QuadPart) / (double)pph()->freq.QuadPart;
}

PPH_FN void pph_init(const char *test, int argc, WCHAR **argv, unsigned w, unsigned h)
{
    struct pph_state *s = pph();
    const char *e;
    int i;
    s->test = test;
    setvbuf(stdout, NULL, _IONBF, 0);
    if ((e = getenv("DX12TEST_SECS")) && *e) s->secs = atoi(e);
    if ((e = getenv("DX12TEST_SYNC")) && *e) s->sync = atoi(e);
    for (i = 1; argv && i + 1 < argc; i++)
    {
        if (!_wcsicmp(argv[i], L"-secs")) s->secs = _wtoi(argv[++i]);
        else if (!_wcsicmp(argv[i], L"-sync")) s->sync = _wtoi(argv[++i]);
    }
    if (s->sync < 0 || s->sync > 4) s->sync = 0;
    QueryPerformanceFrequency(&s->freq);
    QueryPerformanceCounter(&s->init);
    s->start = s->last = s->init;
    printf("DX12: start test=%s secs=%d sync=%d %ux%u\n", s->test, s->secs, s->sync, w, h);
}

PPH_FN UINT pph_sync_interval(void) { return (UINT)pph()->sync; }

PPH_FN void pph_fail(const char *what, HRESULT hr)
{
    printf("DX12: FAIL %s hr=0x%08lx\n", what, (unsigned long)hr);
    fflush(stdout);
}

/* Call after every Present. Returns nonzero once the run time is over; the caller then quits. */
PPH_FN int pph_on_present(HRESULT hr)
{
    struct pph_state *s = pph();
    LARGE_INTEGER t;
    double dt, el;
    if (s->done) return 1;
    QueryPerformanceCounter(&t);
    if (!s->frames)
    {
        s->startup = pph_seconds(s->init, t);
        s->start = s->last = t;
        s->last_frames = 1;
        printf("DX12: first frame startup=%.1f\n", s->startup);
    }
    s->frames++;
    if (s->frames <= 10 || s->frames % 60 == 0)
        printf("DX12: Present frame=%u hr=0x%08lx\n", s->frames, (unsigned long)hr);
    dt = pph_seconds(s->last, t);
    el = pph_seconds(s->start, t);
    if (dt >= 1.0)
    {
        printf("DX12: fps=%.1f frames=%u t=%.1f\n", (s->frames - s->last_frames) / dt, s->frames, el);
        s->last_frames = s->frames;
        s->last = t;
    }
    if (s->secs > 0 && el >= s->secs)
    {
        printf("DX12: PASS test=%s frames=%u secs=%.1f fps=%.1f startup=%.1f\n", s->test, s->frames, el,
               (s->frames - 1) / el, s->startup);
        s->done = 1;
        return 1;
    }
    return 0;
}
#endif
