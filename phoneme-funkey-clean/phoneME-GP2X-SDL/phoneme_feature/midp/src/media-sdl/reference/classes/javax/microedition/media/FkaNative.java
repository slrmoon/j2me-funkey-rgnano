/*
 * FunKey audio engine: native entry points (fkaudio, see fka_kni.c).
 *
 * Players are small integer handles. None of these calls block: the mixer
 * runs in the SDL audio thread and the VM uses green threads, so everything
 * here only takes the audio lock for a moment.
 */
package javax.microedition.media;

final class FkaNative {
    static final int KIND_NONE = 0;
    static final int KIND_PCM = 1;
    static final int KIND_SONG = 2;
    static final int KIND_DEVICE = 3;

    static final int EVENT_END_OF_MEDIA = 1;
    static final int EVENT_STOPPED_AT = 2;

    private FkaNative() {
    }

    static native int nInit();

    static native int nNew(byte[] data, int length, String typeHint);
    static native int nNewDevice();
    static native int nNewToneSequence(byte[] seq);
    static native int nReplaceToneSequence(int id, byte[] seq);
    static native void nDelete(int id);
    static native int nKind(int id);
    static native String nContentType(int id);

    static native int nPrefetch(int id);
    static native void nDeallocate(int id);
    static native int nStart(int id);
    static native void nStop(int id);
    static native boolean nIsPlaying(int id);
    static native void nSetLoopCount(int id, int count);
    static native long nGetTime(int id);
    static native long nSetTime(int id, long us);
    static native long nGetDuration(int id);
    static native void nSetVolume(int id, int level, boolean muted);
    static native void nSetStopTime(int id, long us);
    static native int nSetRate(int id, int milliPercent);
    static native int nGetRate(int id);
    static native int nGetTempo(int id);
    static native int nSetTempo(int id, int milliBpm);
    static native void nSetPitch(int id, int milliSemitones);

    static native void nMidiShort(int id, int status, int d1, int d2);
    static native void nMidiLong(int id, byte[] data, int offset, int length);
    static native int nMidiChannelVolume(int id, int channel);
    static native int nMidiProgram(int id, int channel);
    static native int nMidiBank(int id, int channel);

    static native void nPlayTone(int note, int durationMs, int volume);
    static native int nGetMasterVolume();

    /** Fills out[0..3] = player, type, arg, data; returns false when empty. */
    static native boolean nPollEvent(long[] out);
}
