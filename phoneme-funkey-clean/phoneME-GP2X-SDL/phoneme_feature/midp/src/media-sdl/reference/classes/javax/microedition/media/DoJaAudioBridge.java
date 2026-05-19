package javax.microedition.media;

public final class DoJaAudioBridge {
    private DoJaAudioBridge() {
    }

    public static native int nOpen(byte[] events, int profile);

    public static native void nStart(int id);

    public static native void nStop(int id);

    public static native void nClose(int id);

    public static native void nSetVolume(int id, int volume);

    public static native void nSetLoopCount(int id, int loopCount);
}
