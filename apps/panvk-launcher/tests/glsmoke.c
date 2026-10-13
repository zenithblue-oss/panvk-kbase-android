/* Windowed WGL smoke test: opens an OpenGL context, writes GL_VENDOR/RENDERER/VERSION to glsmoke.txt next to the exe
 * and draws a spinning triangle for ~40 s. Build: tests/build-glsmoke.sh. Used to prove the Render API "OpenGL" path. */
#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <math.h>

static LRESULT CALLBACK proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcA(h, m, w, l);
}

int main(void)
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char *s = strrchr(path, '\\'); if (s) s[1] = 0;
    strcat(path, "glsmoke.txt");
    FILE *log = fopen(path, "w");

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = proc; wc.hInstance = GetModuleHandleA(NULL); wc.lpszClassName = "glsmoke";
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("glsmoke", "glsmoke", WS_POPUP | WS_VISIBLE, 0, 0, 1280, 720, NULL, NULL, wc.hInstance, NULL);
    HDC dc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24;
    int pf = ChoosePixelFormat(dc, &pfd);
    BOOL ok = pf && SetPixelFormat(dc, pf, &pfd);
    HGLRC rc = ok ? wglCreateContext(dc) : NULL;
    if (!rc || !wglMakeCurrent(dc, rc)) {
        if (log) { fprintf(log, "FAIL pf=%d ok=%d rc=%p err=%lu\n", pf, ok, (void *)rc, GetLastError()); fclose(log); }
        return 1;
    }
    if (log) {
        fprintf(log, "vendor=%s\nrenderer=%s\nversion=%s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));
        fclose(log);
    }
    DWORD t0 = GetTickCount();
    MSG msg;
    for (;;) {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) { if (msg.message == WM_QUIT) return 0; DispatchMessageA(&msg); }
        float t = (GetTickCount() - t0) / 1000.0f;
        if (t > 40.0f) break;
        glViewport(0, 0, 1280, 720);
        glClearColor(0.1f, 0.2f + 0.2f * sinf(t), 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();
        glRotatef(t * 90.0f, 0, 0, 1);
        glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0); glVertex2f(-0.6f, -0.5f);
        glColor3f(0, 1, 0); glVertex2f(0.6f, -0.5f);
        glColor3f(1, 1, 0); glVertex2f(0.0f, 0.6f);
        glEnd();
        SwapBuffers(dc);
        Sleep(8);
    }
    wglMakeCurrent(NULL, NULL); wglDeleteContext(rc);
    return 0;
}
