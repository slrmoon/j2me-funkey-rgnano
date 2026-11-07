package javax.microedition.khronos.egl;

/** FunKey: OpenGL ES is not available; only the "none" handle exists. */
final class EGLSurfaceImpl extends EGLSurface {
    private static final EGLSurfaceImpl NONE = new EGLSurfaceImpl();

    private EGLSurfaceImpl() {
    }

    static EGLSurfaceImpl getInstance(int handle) {
        return NONE;
    }
}
