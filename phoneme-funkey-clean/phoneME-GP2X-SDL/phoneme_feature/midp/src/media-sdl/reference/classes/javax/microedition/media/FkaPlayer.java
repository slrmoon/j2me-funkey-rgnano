/*
 * FunKey audio engine: the one Player implementation.
 *
 * Every content type (sampled audio, MIDI and the other melody formats,
 * tone sequences, the interactive MIDI device) is one native fkaudio player,
 * so the state machine, events and controls behave the same for all of
 * them. Native calls never block; asynchronous events (end of media, stop
 * time) come back through FkaEvents.
 */
package javax.microedition.media;

import java.util.Vector;

import javax.microedition.media.control.StopTimeControl;

final class FkaPlayer implements Player {
    static final int TYPE_CONTENT = 0;
    static final int TYPE_TONE_DEVICE = 1;
    static final int TYPE_MIDI_DEVICE = 2;

    private final int id;
    private final int kind;
    private final int origin;
    private final String declaredType;
    private int state = UNREALIZED;
    private int loopCount = 1;
    private long stopTime = StopTimeControl.RESET;
    private final Vector listeners = new Vector(2);
    private Control[] controls;
    private TimeBase timeBase;

    /* control state, shared with FkaControls */
    int level = 100;
    boolean muted;
    int rate = 100000;
    int pitch;

    private FkaPlayer(int id, int origin, String type) {
        this.id = id;
        this.origin = origin;
        this.kind = FkaNative.nKind(id);
        this.declaredType = type;
        FkaEvents.register(this);
    }

    static FkaPlayer fromContent(byte[] data, int length, String type) throws MediaException {
        int id = FkaNative.nNew(data, length, type);
        if (id <= 0) {
            throw new MediaException("Unsupported or corrupt media"
                    + (type != null ? " (" + type + ")" : ""));
        }
        return new FkaPlayer(id, TYPE_CONTENT, type);
    }

    static FkaPlayer toneDevice() throws MediaException {
        int id = FkaNative.nNewToneSequence(null);
        if (id <= 0) throw new MediaException("Cannot create tone player");
        return new FkaPlayer(id, TYPE_TONE_DEVICE, "audio/x-tone-seq");
    }

    static FkaPlayer midiDevice() throws MediaException {
        int id = FkaNative.nNewDevice();
        if (id <= 0) throw new MediaException("Cannot create MIDI player");
        return new FkaPlayer(id, TYPE_MIDI_DEVICE, "audio/midi");
    }

    int nativeId() {
        return id;
    }

    int kind() {
        return kind;
    }

    boolean isToneSequence() {
        return origin == TYPE_TONE_DEVICE
                || "audio/x-tone-seq".equals(FkaNative.nContentType(id));
    }

    private void checkClosed() {
        if (state == CLOSED) throw new IllegalStateException("Player is closed");
    }

    /* ---------------------------------------------------------------- */
    /* life cycle                                                         */
    /* ---------------------------------------------------------------- */

    public synchronized void realize() throws MediaException {
        checkClosed();
        if (state == UNREALIZED) state = REALIZED;
    }

    public synchronized void prefetch() throws MediaException {
        checkClosed();
        if (state == UNREALIZED) realize();
        if (state == REALIZED) {
            FkaNative.nPrefetch(id);
            state = PREFETCHED;
        }
    }

    public void start() throws MediaException {
        long t;
        synchronized (this) {
            checkClosed();
            if (state < PREFETCHED) prefetch();
            if (state == STARTED) {
                /* finished natively but END_OF_MEDIA not processed yet:
                 * games that rewind and start again must hear it */
                if (kind == FkaNative.KIND_DEVICE || FkaNative.nIsPlaying(id)) return;
            }
            if (FkaNative.nStart(id) < 0) throw new MediaException("Cannot start");
            if (state != STARTED) FkaEvents.playerStarted();
            state = STARTED;
            t = mediaTime();
        }
        FkaEvents.post(this, PlayerListener.STARTED, new Long(t));
    }

    public void stop() throws MediaException {
        long t;
        synchronized (this) {
            checkClosed();
            if (state != STARTED) return;
            FkaNative.nStop(id);
            state = PREFETCHED;
            FkaEvents.playerStopped();
            t = mediaTime();
        }
        FkaEvents.post(this, PlayerListener.STOPPED, new Long(t));
    }

    public synchronized void deallocate() {
        checkClosed();
        if (state == STARTED) {
            try {
                stop();
            } catch (MediaException e) {
                /* cannot happen */
            }
        }
        if (state == PREFETCHED) {
            FkaNative.nDeallocate(id);
            state = REALIZED;
        }
    }

    public void close() {
        synchronized (this) {
            if (state == CLOSED) return;
            if (state == STARTED) FkaEvents.playerStopped();
            FkaNative.nDelete(id);
            state = CLOSED;
            FkaEvents.unregister(this);
        }
        FkaEvents.post(this, PlayerListener.CLOSED, null);
    }

    /* native event, from FkaEvents (event thread or a state query) */
    void nativeEvent(int type, long arg, int data) {
        String ev = null, again = null;
        synchronized (this) {
            if (state != STARTED) return;
            if (type == FkaNative.EVENT_END_OF_MEDIA) {
                ev = PlayerListener.END_OF_MEDIA;
                if (data != 0) {
                    again = PlayerListener.STARTED;       /* looped */
                } else if (!FkaNative.nIsPlaying(id)) {
                    state = PREFETCHED;
                    FkaEvents.playerStopped();
                }
            } else if (type == FkaNative.EVENT_STOPPED_AT) {
                ev = PlayerListener.STOPPED_AT_TIME;
                stopTime = StopTimeControl.RESET;
                if (!FkaNative.nIsPlaying(id)) {
                    state = PREFETCHED;
                    FkaEvents.playerStopped();
                }
            }
        }
        if (ev != null) FkaEvents.post(this, ev, new Long(arg));
        if (again != null) FkaEvents.post(this, again, new Long(0));
    }

    /* ---------------------------------------------------------------- */
    /* time                                                               */
    /* ---------------------------------------------------------------- */

    private long mediaTime() {
        if (kind == FkaNative.KIND_DEVICE) return TIME_UNKNOWN;
        return FkaNative.nGetTime(id);
    }

    public synchronized long setMediaTime(long now) throws MediaException {
        checkClosed();
        /* the spec wants IllegalStateException here; handsets were lenient
         * and games rely on it, so realize instead */
        if (state == UNREALIZED) realize();
        if (kind == FkaNative.KIND_DEVICE) throw new MediaException("Not seekable");
        return FkaNative.nSetTime(id, now < 0 ? 0 : now);
    }

    public long getMediaTime() {
        checkClosed();
        return mediaTime();
    }

    public long getDuration() {
        checkClosed();
        if (kind == FkaNative.KIND_DEVICE) return TIME_UNKNOWN;
        long d = FkaNative.nGetDuration(id);
        return d < 0 ? TIME_UNKNOWN : d;
    }

    int rawState() {
        return state;
    }

    public int getState() {
        int s = state;
        if (s == STARTED && kind != FkaNative.KIND_DEVICE && !FkaNative.nIsPlaying(id)) {
            /* playback ended a moment ago: report it now, not one poll later */
            FkaEvents.pump();
            s = state;
        }
        return s;
    }

    public synchronized void setTimeBase(TimeBase master) throws MediaException {
        checkClosed();
        if (state == STARTED) throw new IllegalStateException("Player is started");
        if (master != null && master != Manager.getSystemTimeBase() && master != timeBase) {
            throw new MediaException("TimeBase not supported");
        }
        timeBase = master;
    }

    public TimeBase getTimeBase() {
        checkClosed();
        return timeBase != null ? timeBase : Manager.getSystemTimeBase();
    }

    public String getContentType() {
        checkClosed();
        if (declaredType != null && declaredType.length() > 0) return declaredType;
        return FkaNative.nContentType(id);
    }

    public synchronized void setLoopCount(int count) {
        checkClosed();
        if (count == 0 || count < -1) throw new IllegalArgumentException("Bad loop count " + count);
        if (state == STARTED) throw new IllegalStateException("Player is started");
        loopCount = count;
        FkaNative.nSetLoopCount(id, count);
    }

    int getLoopCount() {
        return loopCount;
    }

    /* ---------------------------------------------------------------- */
    /* listeners                                                          */
    /* ---------------------------------------------------------------- */

    public void addPlayerListener(PlayerListener l) {
        checkClosed();
        if (l != null && !listeners.contains(l)) listeners.addElement(l);
    }

    public void removePlayerListener(PlayerListener l) {
        checkClosed();
        listeners.removeElement(l);
    }

    /* event thread only */
    void deliver(String event, Object data) {
        int n = listeners.size();
        if (n == 0) return;
        PlayerListener[] copy = new PlayerListener[n];
        listeners.copyInto(copy);
        for (int i = 0; i < n; i++) {
            try {
                copy[i].playerUpdate(this, event, data);
            } catch (Throwable t) {
                /* a broken listener must not kill sound for everyone */
            }
        }
    }

    void postVolumeChanged(Object vc) {
        FkaEvents.post(this, PlayerListener.VOLUME_CHANGED, vc);
    }

    /* ---------------------------------------------------------------- */
    /* controls                                                           */
    /* ---------------------------------------------------------------- */

    private Control[] controls() {
        if (controls == null) controls = FkaControls.create(this, origin);
        return controls;
    }

    public Control[] getControls() {
        checkClosed();
        Control[] c = controls();
        Control[] copy = new Control[c.length];
        System.arraycopy(c, 0, copy, 0, c.length);
        return copy;
    }

    public Control getControl(String type) {
        checkClosed();
        if (type == null) throw new IllegalArgumentException("null control type");
        String full = type.indexOf('.') < 0 ? "javax.microedition.media.control." + type : type;
        Control[] c = controls();
        for (int i = 0; i < c.length; i++) {
            if (FkaControls.implementsType(c[i], full)) return c[i];
        }
        return null;
    }

    /* used by StopTimeControl */
    synchronized void setStopTime(long t) {
        checkClosed();
        if (state == STARTED && t != StopTimeControl.RESET && t < mediaTime()) {
            throw new IllegalStateException("Stop time already passed");
        }
        stopTime = t;
        FkaNative.nSetStopTime(id, t == StopTimeControl.RESET ? -1 : t);
    }

    long getStopTime() {
        return stopTime;
    }

    synchronized void applyVolume() {
        if (state != CLOSED) FkaNative.nSetVolume(id, level, muted);
    }
}
