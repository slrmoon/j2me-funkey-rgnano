package javax.microedition.khronos.egl;

/**
 * FunKey: no OpenGL ES (JSR 239) on this device. Every display is
 * EGL_NO_DISPLAY, eglInitialize fails and eglGetError reports
 * EGL_NOT_INITIALIZED, as EGL does for a display it cannot open.
 */
final class EGL10Impl implements EGL11 {
    static final EGL10Impl INSTANCE = new EGL10Impl();

    private EGL10Impl() {
    }

    public int eglGetError() {
        return EGL_NOT_INITIALIZED;
    }

    public EGLDisplay eglGetDisplay(Object native_display) {
        return EGL_NO_DISPLAY;
    }

    public boolean eglInitialize(EGLDisplay display, int[] major_minor) {
        return false;
    }

    public boolean eglTerminate(EGLDisplay display) {
        return true;
    }

    public String eglQueryString(EGLDisplay display, int name) {
        return null;
    }

    public boolean eglGetConfigs(EGLDisplay display, EGLConfig[] configs, int config_size,
            int[] num_config) {
        if (num_config != null && num_config.length > 0) {
            num_config[0] = 0;
        }
        return false;
    }

    public boolean eglChooseConfig(EGLDisplay display, int[] attrib_list, EGLConfig[] configs,
            int config_size, int[] num_config) {
        return eglGetConfigs(display, configs, config_size, num_config);
    }

    public boolean eglGetConfigAttrib(EGLDisplay display, EGLConfig config, int attribute,
            int[] value) {
        return false;
    }

    public EGLSurface eglCreateWindowSurface(EGLDisplay display, EGLConfig config,
            Object native_window, int[] attrib_list) {
        return EGL_NO_SURFACE;
    }

    public EGLSurface eglCreatePixmapSurface(EGLDisplay display, EGLConfig config,
            Object native_pixmap, int[] attrib_list) {
        return EGL_NO_SURFACE;
    }

    public EGLSurface eglCreatePbufferSurface(EGLDisplay display, EGLConfig config,
            int[] attrib_list) {
        return EGL_NO_SURFACE;
    }

    public boolean eglDestroySurface(EGLDisplay display, EGLSurface surface) {
        return false;
    }

    public boolean eglQuerySurface(EGLDisplay display, EGLSurface surface, int attribute,
            int[] value) {
        return false;
    }

    public EGLContext eglCreateContext(EGLDisplay display, EGLConfig config,
            EGLContext share_context, int[] attrib_list) {
        return EGL_NO_CONTEXT;
    }

    public boolean eglDestroyContext(EGLDisplay display, EGLContext context) {
        return false;
    }

    public boolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read,
            EGLContext context) {
        return false;
    }

    public EGLContext eglGetCurrentContext() {
        return EGL_NO_CONTEXT;
    }

    public EGLSurface eglGetCurrentSurface(int readdraw) {
        return EGL_NO_SURFACE;
    }

    public EGLDisplay eglGetCurrentDisplay() {
        return EGL_NO_DISPLAY;
    }

    public boolean eglQueryContext(EGLDisplay display, EGLContext context, int attribute,
            int[] value) {
        return false;
    }

    public boolean eglWaitGL() {
        return true;
    }

    public boolean eglWaitNative(int engine, Object bindTarget) {
        return true;
    }

    public boolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
        return false;
    }

    public boolean eglCopyBuffers(EGLDisplay display, EGLSurface surface, Object native_pixmap) {
        return false;
    }

    public boolean eglSurfaceAttrib(EGLDisplay display, EGLSurface surface, int attribute,
            int value) {
        return false;
    }

    public boolean eglBindTexImage(EGLDisplay display, EGLSurface surface, int buffer) {
        return false;
    }

    public boolean eglReleaseTexImage(EGLDisplay display, EGLSurface surface, int buffer) {
        return false;
    }

    public boolean eglSwapInterval(EGLDisplay display, int interval) {
        return false;
    }
}
