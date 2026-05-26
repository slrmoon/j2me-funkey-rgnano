package com.nttdocomo.ui;

import javax.microedition.media.DoJaAudioBridge;

final class DoJaAudioPlayer {
    private static final int PROFILE_GENERIC_DOJA = 0;
    private static final int PROFILE_NOKIA_S40 = 1;
    private static final int PROFILE_SONY_ERICSSON = 2;
    private static final int PROFILE_GENERIC_GM = 3;

    private int nativeId;
    private int loopCount = 1;
    private int volume = 100;
    private byte[] events;
    private int eventCount;
    private int durationMs;

    DoJaAudioPlayer(byte[] mld, String name) {
        boolean dump = System.getProperty("doja.audio.dump") != null;
        MldPlaybackDiagnostics.dumpComparison(mld, name);
        events = MldEventStream.build(mld, dump);
        if (events == null) {
            throw new IllegalArgumentException("unsupported MLD");
        }
        eventCount = read16(events, 4);
        durationMs = readLastEventTime(events, eventCount);
        nativeId = DoJaAudioBridge.nOpen(events, getProfileId());
        if (nativeId == 0) {
            throw new OutOfMemoryError("DoJa audio");
        }
        System.out.println("DoJaAudioPlayer.open id=" + nativeId
                + " name=" + name
                + " eventBytes=" + events.length
                + " events=" + eventCount
                + " durationMs=" + durationMs
                + " musicLike=" + isMusicLike());
    }

    void realize() {
    }

    void prefetch() {
    }

    void start() {
        if (nativeId != 0) {
            DoJaAudioBridge.nSetLoopCount(nativeId, loopCount);
            DoJaAudioBridge.nSetVolume(nativeId, volume);
            DoJaAudioBridge.nStart(nativeId);
        }
    }

    void stop() {
        if (nativeId != 0) {
            DoJaAudioBridge.nStop(nativeId);
        }
    }

    void close() {
        if (nativeId != 0) {
            int old = nativeId;
            nativeId = 0;
            DoJaAudioBridge.nClose(old);
        }
        events = null;
    }

    void setLoopCount(int count) {
        loopCount = count;
        if (nativeId != 0) {
            DoJaAudioBridge.nSetLoopCount(nativeId, count);
        }
    }

    void setVolume(int level) {
        if (level < 0) {
            level = 0;
        } else if (level > 100) {
            level = 100;
        }
        volume = level;
        if (nativeId != 0) {
            DoJaAudioBridge.nSetVolume(nativeId, level);
        }
    }

    boolean isMusicLike() {
        return eventCount >= 300 || durationMs >= 3000;
    }

    private static int getProfileId() {
        String profile = System.getProperty("doja.audio.profile");
        if ("nokia_s40".equals(profile)) {
            return PROFILE_NOKIA_S40;
        }
        if ("sony_ericsson".equals(profile)) {
            return PROFILE_SONY_ERICSSON;
        }
        if ("generic_gm".equals(profile)) {
            return PROFILE_GENERIC_GM;
        }
        return PROFILE_GENERIC_DOJA;
    }

    private static boolean isDebugEnabled() {
        return "true".equals(System.getProperty("doja.audio.debug"))
                || System.getProperty("doja.audio.dump") != null;
    }

    private static int read16(byte[] data, int pos) {
        if (data == null || pos + 1 >= data.length) {
            return 0;
        }
        return ((data[pos] & 255) << 8) | (data[pos + 1] & 255);
    }

    private static int read32(byte[] data, int pos) {
        if (data == null || pos + 3 >= data.length) {
            return 0;
        }
        return ((data[pos] & 255) << 24) | ((data[pos + 1] & 255) << 16)
                | ((data[pos + 2] & 255) << 8) | (data[pos + 3] & 255);
    }

    private static int readLastEventTime(byte[] data, int count) {
        int last = 0;
        int max = 8 + count * 8;
        if (data == null || max > data.length) {
            max = data == null ? 0 : data.length;
        }
        for (int pos = 8; pos + 8 <= max; pos += 8) {
            int time = read32(data, pos);
            if (time > last) {
                last = time;
            }
        }
        return last;
    }
}
