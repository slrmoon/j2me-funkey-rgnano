package com.nttdocomo.ui;

import java.io.ByteArrayInputStream;
import java.util.Vector;

import javax.microedition.media.Manager;
import javax.microedition.media.Player;
import javax.microedition.media.PlayerListener;
import javax.microedition.media.control.VolumeControl;

import com.funkey.doja.DoJaRuntime;

/**
 * Sound playback through the MMAPI players (fkaudio engine).
 */
public class AudioPresenter implements MediaPresenter {
    public static final int AUDIO_PLAYING = 1;
    public static final int AUDIO_STOPPED = 2;
    public static final int AUDIO_COMPLETE = 3;
    public static final int AUDIO_SYNC = 4;
    public static final int AUDIO_PAUSED = 5;
    public static final int AUDIO_RESTARTED = 6;
    public static final int AUDIO_LOOPED = 7;
    public static final int PRIORITY = 1;
    public static final int SYNC_MODE = 2;
    public static final int TRANSPOSE_KEY = 3;
    public static final int SET_VOLUME = 4;
    public static final int CHANGE_TEMPO = 5;
    public static final int LOOP_COUNT = 6;
    public static final int ATTR_SYNC_OFF = 0;
    public static final int ATTR_SYNC_ON = 1;
    public static final int MIN_PRIORITY = 1;
    public static final int NORM_PRIORITY = 5;
    public static final int MAX_PRIORITY = 10;
    public static final int MIN_OPTION_ATTR = 128;
    public static final int MAX_OPTION_ATTR = 255;
    protected static final int MIN_VENDOR_ATTR = 64;
    protected static final int MAX_VENDOR_ATTR = 127;
    protected static final int MIN_VENDOR_AUDIO_EVENT = 64;
    protected static final int MAX_VENDOR_AUDIO_EVENT = 127;

    private static final Vector all = new Vector();
    private static boolean audioBroken;

    private MediaResource resource;
    private MediaListener listener;
    private int volume = 100;
    private int loopCount;
    private Player player;
    private MediaResource playerFor;
    private volatile boolean playing;
    private volatile int generation;

    protected AudioPresenter() {
        synchronized (all) {
            all.addElement(this);
        }
    }

    protected AudioPresenter(int port) {
        this();
    }

    public static AudioPresenter getAudioPresenter() {
        return new AudioPresenter();
    }

    /*
     * DoJa-4.0 guarantees at least four presenters on explicit ports and
     * throws for a port past the last one; games probe the count by calling
     * getAudioPresenter(n) until it throws (THE Battle Cats). Same default as
     * openDoJa, -Ddoja.audio.ports=N to change.
     */
    private static int portCount() {
        try {
            String v = System.getProperty("doja.audio.ports");
            if (v != null) {
                return Math.max(1, Integer.parseInt(v.trim()));
            }
        } catch (RuntimeException e) {
        }
        return 4;
    }

    public static AudioPresenter getAudioPresenter(int port) {
        if (port < 0 || port >= portCount()) {
            throw new IllegalArgumentException("port");
        }
        return new AudioPresenter(port);
    }

    /** runtime: stop every sound (application exit). */
    public static void stopAll() {
        synchronized (all) {
            for (int i = 0; i < all.size(); i++) {
                ((AudioPresenter) all.elementAt(i)).release();
            }
        }
    }

    public void setSound(MediaSound sound) {
        setResource(sound);
    }

    public void setData(MediaData data) {
        setResource(data);
    }

    private void setResource(MediaResource r) {
        if (playing) {
            stop();
        }
        resource = r;
    }

    public MediaResource getMediaResource() {
        return resource;
    }

    public void play() {
        play(0);
    }

    public synchronized void play(int time) {
        if (time < 0) {
            throw new IllegalArgumentException("time");
        }
        final int gen = ++generation;
        stopPlayer();
        Player p = prepare();
        if (p == null) {
            playing = false;
            notify(AUDIO_PLAYING, 0, gen);
            notify(AUDIO_COMPLETE, 0, gen);
            return;
        }
        try {
            p.setLoopCount(loopCount < 0 ? -1 : loopCount + 1);
        } catch (Throwable t) {
            /* already started: ignore */
        }
        try {
            if (time > 0) {
                p.setMediaTime(time * 1000L);
            } else {
                p.setMediaTime(0);
            }
        } catch (Throwable t) {
        }
        applyVolume(p);
        try {
            playing = true;
            p.start();
            notify(AUDIO_PLAYING, 0, gen);
        } catch (Throwable t) {
            System.out.println("DoJa: audio start failed: " + t);
            playing = false;
            notify(AUDIO_PLAYING, 0, gen);
            notify(AUDIO_COMPLETE, 0, gen);
        }
    }

    private Player prepare() {
        if (audioBroken) {
            return null;
        }
        MediaResource r = resource;
        if (!(r instanceof MediaManager.Res)) {
            return null;
        }
        if (player != null && playerFor == r) {
            return player;
        }
        closePlayer();
        MediaManager.Res res = (MediaManager.Res) r;
        byte[] data;
        try {
            if (!res.used) {
                res.use();
            }
            data = res.bytes();
        } catch (Throwable t) {
            System.out.println("DoJa: sound not available: " + t);
            return null;
        }
        if (data == null || data.length < 4) {
            return null;
        }
        String type = "audio/midi";
        byte[] play = data;
        if (data[0] == 'm' && data[1] == 'e' && data[2] == 'l' && data[3] == 'o') {
            /* native MA-3 synthesis in fkaudio */
            if (com.funkey.doja.DoJaRuntime.TRACE) {
                System.out.println("DoJa trace: MLD sound bytes=" + data.length);
            }
            Player direct = createPrepared(data, "audio/x-mld", r);
            if (direct != null) {
                return direct;
            }
            System.out.println("DoJa: native MLD failed, using the MIDI conversion");
            if (r instanceof MediaManager.FkMediaSound) {
                MediaManager.FkMediaSound s = (MediaManager.FkMediaSound) r;
                if (s.midi == null && s.wav == null) {
                    s.midi = MldPcmDecoder.decodeToMidi(data);
                    if (s.midi == null) {
                        s.wav = MldPcmDecoder.decodeToWav(data);
                    }
                }
                if (s.midi != null) {
                    play = s.midi;
                } else if (s.wav != null) {
                    play = s.wav;
                    type = "audio/x-wav";
                } else {
                    return null;
                }
            } else {
                play = MldPcmDecoder.decodeToMidi(data);
                if (play == null) {
                    return null;
                }
            }
        } else if (data[0] == 'R' && data[1] == 'I') {
            type = "audio/x-wav";
        } else if (data[0] == 'M' && data[1] == 'M' && data[2] == 'M' && data[3] == 'D') {
            type = "application/vnd.smaf";
        }
        return createPrepared(play, type, r);
    }

    private Player createPrepared(byte[] play, String type, MediaResource r) {
        try {
            Player p = Manager.createPlayer(new ByteArrayInputStream(play), type);
            p.realize();
            p.prefetch();
            p.addPlayerListener(new PlayerListener() {
                public void playerUpdate(Player pl, String event, Object data) {
                    if (PlayerListener.END_OF_MEDIA.equals(event)) {
                        if (pl.getState() == Player.STARTED) {
                            looped(pl);     /* more plays follow */
                        } else {
                            ended(pl);
                        }
                    }
                }
            });
            player = p;
            playerFor = r;
            return p;
        } catch (Throwable t) {
            System.out.println("DoJa: audio player failed (" + type + "): " + t);
            if (t instanceof javax.microedition.media.MediaException
                    && t.getMessage() != null && t.getMessage().indexOf("initialize") >= 0) {
                audioBroken = true;
            }
            return null;
        }
    }

    private void ended(Player pl) {
        final int gen;
        synchronized (this) {
            if (pl != player || !playing) {
                return;
            }
            playing = false;
            gen = generation;
        }
        notify(AUDIO_COMPLETE, 0, gen);
    }

    private void looped(Player pl) {
        if (pl == player && playing) {
            notify(AUDIO_LOOPED, 0, generation);
        }
    }

    private void applyVolume(Player p) {
        try {
            Object c = p.getControl("VolumeControl");
            if (c instanceof VolumeControl) {
                /* DoJa volume is a linear gain, MMAPI levels follow a
                 * square law in fkaudio */
                int level = (int) (Math.sqrt(volume / 100.0) * 100.0 + 0.5);
                ((VolumeControl) c).setLevel(level);
            }
        } catch (Throwable t) {
        }
    }

    public void pause() {
        Player p = player;
        if (p != null && playing) {
            try {
                p.stop();
            } catch (Throwable t) {
            }
            notify(AUDIO_PAUSED, 0, generation);
        }
    }

    public void restart() {
        Player p = player;
        if (p != null && playing) {
            try {
                p.start();
            } catch (Throwable t) {
            }
            notify(AUDIO_RESTARTED, 0, generation);
        }
    }

    public int getCurrentTime() {
        Player p = player;
        if (p == null) {
            return 0;
        }
        try {
            long t = p.getMediaTime();
            return t < 0 ? 0 : (int) (t / 1000);
        } catch (Throwable t) {
            return 0;
        }
    }

    public int getTotalTime() {
        Player p = player;
        if (p == null) {
            p = prepare();
        }
        if (p == null) {
            return 0;
        }
        try {
            long t = p.getDuration();
            return t < 0 ? 0 : (int) (t / 1000);
        } catch (Throwable t) {
            return 0;
        }
    }

    public void setSyncEvent(int channel, int key) {
    }

    public synchronized void stop() {
        boolean was = playing;
        generation++;
        stopPlayer();
        if (was) {
            notify(AUDIO_STOPPED, 0, generation);
        }
    }

    private void stopPlayer() {
        playing = false;
        Player p = player;
        if (p != null) {
            try {
                p.stop();
            } catch (Throwable t) {
            }
        }
    }

    private void closePlayer() {
        Player p = player;
        player = null;
        playerFor = null;
        if (p != null) {
            try {
                p.close();
            } catch (Throwable t) {
            }
        }
    }

    synchronized void release() {
        playing = false;
        closePlayer();
    }

    public void setAttribute(int key, int value) {
        if (key == SET_VOLUME) {
            if (value < 0) {
                value = 0;
            } else if (value > 100) {
                value = 100;
            }
            volume = value;
            Player p = player;
            if (p != null) {
                applyVolume(p);
            }
        } else if (key == LOOP_COUNT) {
            loopCount = value;
        } else if (key == CHANGE_TEMPO || key == TRANSPOSE_KEY) {
            com.funkey.doja.Stub.log("AudioPresenter.setAttribute(" + key + ")");
        }
    }

    public void setMediaListener(MediaListener l) {
        listener = l;
    }

    private void notify(final int type, final int param, final int gen) {
        final MediaListener l = listener;
        if (l == null) {
            return;
        }
        DoJaRuntime.postRun(new Runnable() {
            public void run() {
                if (type != AUDIO_PLAYING && type != AUDIO_STOPPED && gen != generation) {
                    return;
                }
                l.mediaAction(AudioPresenter.this, type, param);
            }
        });
    }
}
