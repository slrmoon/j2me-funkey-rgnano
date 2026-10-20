/*
 * Nokia UI API sound, on top of the FunKey MMAPI implementation.
 *
 * FORMAT_TONE data is a Nokia Smart Messaging (OTA) ringtone, FORMAT_WAV is
 * sampled audio; in practice games pass MIDI or other formats under both,
 * which is fine because the player detects the format from the bytes.
 */
package com.nokia.mid.sound;

import java.io.ByteArrayInputStream;
import java.io.IOException;

import javax.microedition.media.Manager;
import javax.microedition.media.MediaException;
import javax.microedition.media.Player;
import javax.microedition.media.PlayerListener;
import javax.microedition.media.control.ToneControl;
import javax.microedition.media.control.VolumeControl;

public class Sound {
    public static final int FORMAT_TONE = 1;
    public static final int FORMAT_WAV = 5;
    public static final int SOUND_PLAYING = 0;
    public static final int SOUND_STOPPED = 1;
    public static final int SOUND_UNINITIALIZED = 3;

    private Player player;
    private int state = SOUND_UNINITIALIZED;
    private int gain = 255;
    private SoundListener listener;

    private final PlayerListener events = new PlayerListener() {
        public void playerUpdate(Player p, String event, Object data) {
            if (p != player) return;
            if (PlayerListener.STARTED.equals(event)) {
                changed(SOUND_PLAYING);
            } else if (PlayerListener.STOPPED.equals(event)
                    || PlayerListener.STOPPED_AT_TIME.equals(event)) {
                changed(SOUND_STOPPED);
            } else if (PlayerListener.END_OF_MEDIA.equals(event)) {
                if (p.getState() != Player.STARTED) changed(SOUND_STOPPED);
            }
        }
    };

    public Sound(int freq, long duration) {
        init(freq, duration);
    }

    public Sound(byte[] data, int type) {
        init(data, type);
    }

    public static int getConcurrentSoundCount(int type) {
        return type == FORMAT_WAV ? 8 : 1;
    }

    public static int[] getSupportedFormats() {
        return new int[] {FORMAT_TONE, FORMAT_WAV};
    }

    public void init(int freq, long duration) {
        if (duration <= 0 || freq < 0) {
            throw new IllegalArgumentException("Bad tone " + freq + " Hz, " + duration + " ms");
        }
        releasePlayer();
        try {
            Player p = Manager.createPlayer(Manager.TONE_DEVICE_LOCATOR);
            p.realize();
            ToneControl tc = (ToneControl) p.getControl("ToneControl");
            tc.setSequence(singleTone(freq == 0 ? ToneControl.SILENCE : freqToNote(freq), duration));
            attach(p);
        } catch (IOException e) {
            throw new IllegalArgumentException(e.toString());
        } catch (MediaException e) {
            throw new IllegalArgumentException(e.toString());
        }
    }

    public void init(byte[] data, int type) {
        if (data == null) throw new NullPointerException();
        if (type != FORMAT_TONE && type != FORMAT_WAV) {
            throw new IllegalArgumentException("Unsupported format " + type);
        }
        releasePlayer();
        try {
            attach(Manager.createPlayer(new ByteArrayInputStream(data),
                    type == FORMAT_TONE ? "audio/x-nokia-ota" : "audio/x-wav"));
        } catch (IOException e) {
            throw new IllegalArgumentException(e.toString());
        } catch (MediaException e) {
            throw new IllegalArgumentException(e.toString());
        }
    }

    private void attach(Player p) throws MediaException {
        p.addPlayerListener(events);
        p.prefetch();
        player = p;
        applyGain();
        changed(SOUND_STOPPED);
    }

    private void releasePlayer() {
        if (player != null) {
            Player p = player;
            player = null;
            p.close();
        }
    }

    public int getState() {
        return state;
    }

    public int getGain() {
        return gain;
    }

    public void setGain(int gain) {
        if (gain < 0) gain = 0;
        if (gain > 255) gain = 255;
        this.gain = gain;
        applyGain();
    }

    private void applyGain() {
        if (player == null) return;
        VolumeControl vc = (VolumeControl) player.getControl("VolumeControl");
        if (vc != null) vc.setLevel((gain * 100 + 127) / 255);
    }

    /** loop: 1..255 plays, 0 forever. */
    public void play(int loop) {
        if (loop < 0 || loop > 255) throw new IllegalArgumentException("loop = " + loop);
        if (player == null) throw new IllegalStateException("Sound released");
        try {
            if (player.getState() == Player.STARTED) player.stop();
            player.setMediaTime(0);
            player.setLoopCount(loop == 0 ? -1 : loop);
            player.start();
        } catch (MediaException e) {
            /* keep the game running without this sound */
        }
    }

    public void resume() {
        if (player == null) return;
        try {
            player.start();
        } catch (MediaException e) {
            /* ignore */
        }
    }

    public void stop() {
        if (player == null) return;
        try {
            player.stop();
        } catch (MediaException e) {
            /* ignore */
        }
    }

    public void release() {
        releasePlayer();
        changed(SOUND_UNINITIALIZED);
    }

    public void setSoundListener(SoundListener l) {
        listener = l;
    }

    private void changed(int s) {
        if (state == s) return;
        state = s;
        SoundListener l = listener;
        if (l != null) {
            try {
                l.soundStateChanged(this, s);
            } catch (Throwable t) {
                /* ignore */
            }
        }
    }

    /* ---------------------------------------------------------------- */

    static int freqToNote(int hz) {
        double f = 440.0;
        int note = 69;
        while (hz > f * 1.0293 && note < 127) {
            f *= 1.0594630943592953;
            note++;
        }
        while (hz < f / 1.0293 && note > 0) {
            f /= 1.0594630943592953;
            note--;
        }
        return note;
    }

    /* One note as a JSR-135 tone sequence: picks tempo and resolution so
     * that a single duration value (<= 127) is as exact as possible. */
    static byte[] singleTone(int note, long ms) {
        long us = ms * 1000L;
        int bestT = 30, bestRes = 64, bestU = 1;
        long bestErr = Long.MAX_VALUE;
        for (int res = 127; res >= 1 && bestErr > us / 200; res--) {
            for (int t = 127; t >= 5; t--) {
                long unit = 60000000L / ((long) t * res);
                long u = (us + unit / 2) / unit;
                if (u < 1 || u > 127) continue;
                long err = Math.abs(u * unit - us);
                if (err < bestErr) {
                    bestErr = err;
                    bestT = t;
                    bestRes = res;
                    bestU = (int) u;
                }
            }
        }
        return new byte[] {
            ToneControl.VERSION, 1,
            ToneControl.TEMPO, (byte) bestT,
            ToneControl.RESOLUTION, (byte) bestRes,
            (byte) note, (byte) bestU
        };
    }
}
