package com.nokia.mid.sound;

import java.io.ByteArrayInputStream;
import javax.microedition.media.Manager;
import javax.microedition.media.MediaException;
import javax.microedition.media.Player;

public class Sound {
    public static final int FORMAT_TONE = 1;
    public static final int FORMAT_WAV = 5;

    public static final int SOUND_UNINITIALIZED = 0;
    public static final int SOUND_STOPPED = 1;
    public static final int SOUND_PLAYING = 2;

    private int state = SOUND_STOPPED;
    private int gain = 100;
    private SoundListener listener;
    private int type = FORMAT_TONE;
    private byte[] data;
    private int frequency;
    private long duration;
    private Player player;

    private static final int[] MIDI_NOTE_FREQ = {
        8,8,9,9,10,10,11,12,12,13,14,15,16,17,18,19,
        20,21,23,24,25,27,29,30,32,34,36,38,41,43,46,49,
        51,55,58,61,65,69,73,77,82,87,92,98,103,110,116,123,
        130,138,146,155,164,174,185,196,207,220,233,246,261,277,293,311,
        329,349,369,392,415,440,466,493,523,554,587,622,659,698,739,783,
        830,880,932,987,1046,1108,1174,1244,1318,1396,1479,1567,1661,1760,1864,1975,
        2093,2217,2349,2489,2637,2793,2959,3135,3322,3520,3729,3951,4186,4434,4698,4978,
        5274,5587,5919,6271,6644,7040,7458,7902,8372,8869,9397,9956,10548,11175,11839,12543
    };

    public Sound(byte[] data, int type) {
        init(data, type);
    }

    public Sound(int frequency, long duration) {
        init(frequency, duration);
    }

    public void init(byte[] data, int type) {
        this.data = data;
        this.type = type;
        this.frequency = 0;
        this.duration = 0;
        state = SOUND_STOPPED;
        log("init bytes type=" + type + " length=" +
                (data == null ? 0 : data.length));
    }

    public void init(int frequency, long duration) {
        this.type = FORMAT_TONE;
        this.data = null;
        this.frequency = frequency;
        this.duration = duration;
        state = SOUND_STOPPED;
        log("init tone frequency=" + frequency + " duration=" + duration);
    }

    public void play(int loop) {
        stopPlayer();
        state = SOUND_PLAYING;
        log("play type=" + type + " loop=" + loop + " gain=" + gain +
                " frequency=" + frequency + " duration=" + duration);
        notifyListener();
        if (type == FORMAT_TONE && frequency > 0 && duration > 0) {
            try {
                Manager.playTone(frequencyToMidiNote(frequency),
                        (int)duration, gain);
                scheduleStop(duration);
                return;
            } catch (MediaException e) {
                log("play tone failed " + e.getMessage());
            }
        } else if (type == FORMAT_WAV && data != null && data.length > 0) {
            try {
                player = Manager.createPlayer(new ByteArrayInputStream(data),
                        "audio/x-wav");
                player.realize();
                player.setLoopCount(loop <= 0 ? -1 : loop);
                player.start();
                return;
            } catch (Exception e) {
                log("play wav failed " + e.getMessage());
            }
        } else {
            log("play bytes unsupported type=" + type);
        }
        finishPlayback();
    }

    public void stop() {
        stopPlayer();
        state = SOUND_STOPPED;
        log("stop");
        notifyListener();
    }

    public void resume() {
        log("resume");
        play(1);
    }

    public int getState() {
        return state;
    }

    public int getGain() {
        return gain;
    }

    public void setGain(int gain) {
        if (gain < 0) {
            gain = 0;
        }
        if (gain > 100) {
            gain = 100;
        }
        this.gain = gain;
        log("setGain gain=" + gain);
    }

    public void setSoundListener(SoundListener listener) {
        this.listener = listener;
        log("setSoundListener present=" + (listener != null));
    }

    private void notifyListener() {
        if (listener != null) {
            listener.soundStateChanged(this, state);
        }
    }

    private void scheduleStop(final long delay) {
        new Thread(new Runnable() {
            public void run() {
                try {
                    Thread.sleep(delay);
                } catch (InterruptedException e) {
                }
                finishPlayback();
            }
        }).start();
    }

    private void finishPlayback() {
        stopPlayer();
        state = SOUND_STOPPED;
        notifyListener();
    }

    private void stopPlayer() {
        if (player != null) {
            try {
                player.stop();
            } catch (MediaException e) {
            }
            player.close();
            player = null;
        }
    }

    private int frequencyToMidiNote(int hz) {
        int i;
        int best = 0;
        int bestDiff = 2147483647;
        int diff;
        if (hz <= 0) {
            return 0;
        }
        for (i = 0; i < MIDI_NOTE_FREQ.length; i++) {
            diff = MIDI_NOTE_FREQ[i] - hz;
            if (diff < 0) {
                diff = -diff;
            }
            if (diff < bestDiff) {
                bestDiff = diff;
                best = i;
            }
        }
        return best;
    }

    private static void log(String message) {
        System.out.println("Nokia sound: " + message);
    }
}
