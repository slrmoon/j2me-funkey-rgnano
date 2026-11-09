package javax.microedition.khronos.egl;

/** FunKey: OpenGL ES is not available; only the "none" handle exists. */
final class EGLContextImpl extends EGLContext {
    private static final EGLContextImpl NONE = new EGLContextImpl();

    private EGLContextImpl() {
    }

    static EGLContextImpl getInstance(int handle) {
        return NONE;
    }

    public javax.microedition.khronos.opengles.GL getGL() {
        return null;
    }
}
