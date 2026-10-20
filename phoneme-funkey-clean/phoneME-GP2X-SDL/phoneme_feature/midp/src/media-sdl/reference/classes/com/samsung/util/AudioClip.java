/*
 * Samsung sound API, on top of the FunKey MMAPI implementation.
 */
package com.samsung.util;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

import javax.microedition.media.Manager;
import javax.microedition.media.MediaException;
import javax.microedition.media.Player;
import javax.microedition.media.control.VolumeControl;

public class AudioClip {
    public static final int TYPE_MMF = 1;
    public static final int TYPE_MP3 = 2;
    public static final int TYPE_MIDI = 3;

    private static final String[] TYPES = {null, "application/vnd.smaf", "audio/mpeg", "audio/midi"};

    private Player player;

    public AudioClip(int type, byte[] audioData, int audioOffset, int audioLength) {
        if (audioData == null) throw new NullPointerException();
        if (audioOffset < 0 || audioLength < 0 || audioOffset + audioLength > audioData.length) {
            throw new ArrayIndexOutOfBoundsException();
        }
        open(new ByteArrayInputStream(audioData, audioOffset, audioLength), type);
    }

    public AudioClip(int type, String filename) throws IOException {
        if (filename == null) throw new NullPointerException();
        String path = filename.startsWith("/") ? filename : "/" + filename;
        InputStream in = getClass().getResourceAsStream(path);
        if (in == null) throw new IOException("Not found: " + filename);
        try {
            open(in, type);
        } finally {
            in.close();
        }
    }

    private void open(InputStream in, int type) {
        /* some games pass a track number as the type: the data decides */
        String mime = type >= 1 && type <= 3 ? TYPES[type] : null;
        try {
            player = Manager.createPlayer(in, mime);
            player.prefetch();
        } catch (IOException e) {
            player = null;
        } catch (MediaException e) {
            player = null;
        }
    }

    public static boolean isSupported() {
        return true;
    }

    /** loop: 0 forever, 1..255 plays; volume 0..5. */
    public void play(int loop, int volume) {
        if (loop < 0 || loop > 255 || volume < 0 || volume > 5) throw new IllegalArgumentException();
        if (player == null) return;
        try {
            if (player.getState() == Player.STARTED) player.stop();
            player.setMediaTime(0);
            player.setLoopCount(loop == 0 ? -1 : loop);
            VolumeControl vc = (VolumeControl) player.getControl("VolumeControl");
            if (vc != null) vc.setLevel(volume * 20);
            player.start();
        } catch (MediaException e) {
            /* ignore */
        }
    }

    public void pause() {
        stop();
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
}
