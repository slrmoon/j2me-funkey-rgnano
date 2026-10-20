/*
 * Motorola MIDI API (one background tune at a time and note effects),
 * on top of the FunKey MMAPI implementation.
 */
package com.motorola.midi;

import java.io.InputStream;
import java.util.Timer;
import java.util.TimerTask;

import javax.microedition.media.Manager;
import javax.microedition.media.Player;
import javax.microedition.media.control.MIDIControl;

public class MidiPlayer {
    public static final int PLAY_CONTINUOUS = 0;
    public static final int PLAY_ONCE = 1;

    private static Player player;
    private static Player effects;
    private static Timer timer;

    private MidiPlayer() {
    }

    public static synchronized void play(String filename, int mode) {
        stop();
        if (filename == null) return;
        int slash = filename.indexOf('/');
        String path = slash >= 0 ? filename.substring(slash) : "/" + filename;
        try {
            InputStream in = MidiPlayer.class.getResourceAsStream(path);
            if (in == null) return;
            try {
                player = Manager.createPlayer(in, "audio/midi");
            } finally {
                in.close();
            }
            if (mode == PLAY_CONTINUOUS) player.setLoopCount(-1);
            player.start();
        } catch (Exception e) {
            player = null;
        }
    }

    public static synchronized void stop() {
        if (player != null) {
            player.close();
            player = null;
        }
    }

    /** One note on a MIDI channel: velocity, channel volume, program. */
    public static synchronized void playEffect(int note, int keyPressure, int channelVol,
                                               int instrument, int channel) {
        try {
            if (effects == null) {
                effects = Manager.createPlayer(Manager.MIDI_DEVICE_LOCATOR);
                effects.prefetch();
            }
            MIDIControl mc = (MIDIControl) effects.getControl("MIDIControl");
            int ch = channel & 15;
            mc.setProgram(ch, -1, instrument & 127);
            mc.setChannelVolume(ch, channelVol & 127);
            final MIDIControl fmc = mc;
            final int fch = ch, fnote = note & 127;
            fmc.shortMidiEvent(MIDIControl.NOTE_ON | ch, fnote, keyPressure & 127);
            /* the effect is a short blip: release it after a moment */
            if (timer == null) timer = new Timer();
            timer.schedule(new TimerTask() {
                public void run() {
                    try {
                        fmc.shortMidiEvent(0x80 | fch, fnote, 0);
                    } catch (Exception e) {
                        /* ignore */
                    }
                }
            }, 400);
        } catch (Exception e) {
            /* ignore */
        }
    }
}
