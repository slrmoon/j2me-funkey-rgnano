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

    DoJaAudioPlayer(byte[] mld, String name) {
        boolean dump = System.getProperty("doja.audio.dump") != null;
        events = MldEventStream.build(mld, dump);
        if (events == null) {
            throw new IllegalArgumentException("unsupported MLD");
        }
        nativeId = DoJaAudioBridge.nOpen(events, getProfileId());
        if (nativeId == 0) {
            throw new OutOfMemoryError("DoJa audio");
        }
        System.out.println("DoJaAudioPlayer.open name=" + name + " eventBytes=" + events.length);
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
}
