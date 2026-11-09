package javax.microedition.khronos.egl;

/** FunKey: OpenGL ES is not available; only the "none" handle exists. */
final class EGLDisplayImpl extends EGLDisplay {
    private static final EGLDisplayImpl NONE = new EGLDisplayImpl();

    private EGLDisplayImpl() {
    }

    static EGLDisplayImpl getInstance(int handle) {
        return NONE;
    }
}
