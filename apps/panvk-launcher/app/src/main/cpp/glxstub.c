/*
 * libGL.so.1 stand-in (no GLX here). The imagefs libgstgl-1.0 has NEEDED libGL.so.1, and Proton's
 * winegstreamer.so links libgstgl, so without a libGL.so.1 winegstreamer fails to dlopen and Wine has no
 * WMA decoder: XAudio2 xWMA sounds (Fallout 4 music, voices, most effects) stay silent.
 * These are the GLX entry points libgstgl imports; every one reports "no GLX", so GStreamer's GL
 * elements and winex11's GLX path both disable themselves. PanPlay links it as files/glxstub/libGL.so.1.
 */
#define STUB(name) __attribute__((visibility("default"))) void *name(void) { return 0; }
STUB(glXChooseFBConfig)
STUB(glXChooseVisual)
STUB(glXCreateContext)
STUB(glXDestroyContext)
STUB(glXGetCurrentContext)
STUB(glXGetFBConfigAttrib)
STUB(glXGetFBConfigs)
STUB(glXGetProcAddressARB)
STUB(glXGetProcAddress)
STUB(glXGetVisualFromFBConfig)
STUB(glXMakeCurrent)
STUB(glXQueryContext)
STUB(glXQueryExtension)
STUB(glXQueryExtensionsString)
STUB(glXQueryVersion)
STUB(glXSwapBuffers)
