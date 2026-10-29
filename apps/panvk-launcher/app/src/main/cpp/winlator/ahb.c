// panvk-launcher "EGL" present path: X window drawables live in AHardwareBuffers that the compositor samples through an
// EGLImage (no glTexSubImage2D upload per frame). Pixels stay X11 BGRA in an RGBA8 buffer; Texture swizzles R/B.
#include <jni.h>
#include <string.h>
#include <android/hardware_buffer.h>
#include <android/log.h>
#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define TAG "PanPlayEGL"
#define USAGE (AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN)

// AHB*, or 0 when allocation fails or the row stride is not the width (Drawable code assumes stride == width).
JNIEXPORT jlong JNICALL
Java_com_winlator_xserver_Drawable_ahbAllocate(JNIEnv *env, jclass cls, jint width, jint height) {
    AHardwareBuffer_Desc d = { .width = (uint32_t)width, .height = (uint32_t)height, .layers = 1,
                               .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM, .usage = USAGE };
    AHardwareBuffer *b = NULL;
    if (AHardwareBuffer_allocate(&d, &b) != 0 || !b) return 0;
    AHardwareBuffer_describe(b, &d);
    if (d.stride != (uint32_t)width) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "%dx%d stride %u != width: CPU drawable", width, height, d.stride);
        AHardwareBuffer_release(b);
        return 0;
    }
    return (jlong)b;
}

// Locks for CPU access and wraps the mapping; null on failure.
JNIEXPORT jobject JNICALL
Java_com_winlator_xserver_Drawable_ahbLock(JNIEnv *env, jclass cls, jlong ahb) {
    AHardwareBuffer *b = (AHardwareBuffer *)ahb;
    AHardwareBuffer_Desc d;
    AHardwareBuffer_describe(b, &d);
    void *p = NULL;
    if (AHardwareBuffer_lock(b, USAGE & ~AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, -1, NULL, &p) != 0 || !p) return NULL;
    memset(p, 0, (size_t)d.width * d.height * 4); // heap drawables start zeroed too
    return (*env)->NewDirectByteBuffer(env, p, (jlong)d.width * d.height * 4);
}

// Unlock (cache clean, so the GPU sees the CPU writes) and lock again. Returns null when the mapping stayed put
// (gralloc keeps it), else a buffer for the new address (or for nothing: capacity 0 when the relock failed).
JNIEXPORT jobject JNICALL
Java_com_winlator_xserver_Drawable_ahbSync(JNIEnv *env, jclass cls, jlong ahb, jobject data) {
    AHardwareBuffer *b = (AHardwareBuffer *)ahb;
    void *old = (*env)->GetDirectBufferAddress(env, data), *p = NULL;
    AHardwareBuffer_unlock(b, NULL);
    if (AHardwareBuffer_lock(b, USAGE & ~AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, -1, NULL, &p) != 0 || !p) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "relock failed");
        return NULL;
    }
    if (p == old) return NULL;
    __android_log_print(ANDROID_LOG_WARN, TAG, "mapping moved %p -> %p", old, p);
    return (*env)->NewDirectByteBuffer(env, p, (*env)->GetDirectBufferCapacity(env, data));
}

JNIEXPORT void JNICALL
Java_com_winlator_xserver_Drawable_ahbRelease(JNIEnv *env, jclass cls, jlong ahb) {
    AHardwareBuffer *b = (AHardwareBuffer *)ahb;
    AHardwareBuffer_unlock(b, NULL);
    AHardwareBuffer_release(b);
}

// Binds [ahb] to the GL_TEXTURE_2D currently bound; returns the EGLImage (0 on failure). GL thread only.
JNIEXPORT jlong JNICALL
Java_com_winlator_renderer_Texture_bindAhb(JNIEnv *env, jclass cls, jlong ahb) {
    EGLDisplay dpy = eglGetCurrentDisplay();
    EGLClientBuffer cb = eglGetNativeClientBufferANDROID((AHardwareBuffer *)ahb);
    if (!cb) return 0;
    const EGLint attrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    EGLImageKHR img = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, cb, attrs);
    if (img == EGL_NO_IMAGE_KHR) {
        __android_log_print(ANDROID_LOG_WARN, TAG, "eglCreateImageKHR failed 0x%x", eglGetError());
        return 0;
    }
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)img);
    return (jlong)img;
}

JNIEXPORT void JNICALL
Java_com_winlator_renderer_Texture_destroyImage(JNIEnv *env, jclass cls, jlong img) {
    eglDestroyImageKHR(eglGetCurrentDisplay(), (EGLImageKHR)img);
}
