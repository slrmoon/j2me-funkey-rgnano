package com.nttdocomo.opt.device;

import com.nttdocomo.lang.XString;

/**
 * Text to speech (F884i and other "raku-raku" models). There is no voice
 * here: the synthesizer exists, takes its settings and reports each text as
 * spoken at once, so games that read their menus aloud (Brain Training, memo
 * apps) run silently instead of stopping with a NullPointerException.
 */
public class SpeechSynthesizer {
    public static final int SPEECH_VOICE = 0;
    public static final int SPEECH_SPEED = 1;
    public static final int MALE_VOICE = 0;
    public static final int FEMALE_VOICE = 1;
    public static final int DEFAULT_VOICE = 2;
    public static final int MIN_SPEED = 1;
    public static final int MAX_SPEED = 6;

    private static SpeechSynthesizer instance;
    private SpeechListener listener;
    private int voice = DEFAULT_VOICE;
    private int speed = 3;

    protected SpeechSynthesizer() {
    }

    public static synchronized SpeechSynthesizer getSpeechSynthesizer() {
        if (instance == null) {
            instance = new SpeechSynthesizer();
        }
        return instance;
    }

    public synchronized void setSpeechListener(SpeechListener listener) {
        this.listener = listener;
    }

    private void done(final int state) {
        final SpeechListener l = listener;
        if (l == null) {
            return;
        }
        /* from another thread, as the handset's speech engine reports */
        new Thread() {
            public void run() {
                try {
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                }
                l.speechAction(state);
            }
        }.start();
    }

    public void speak(String words) {
        /* null: games clear the queue with it (Healthy Life Journal) */
        done(SpeechListener.SPEECH_COMPLETE);
    }

    public void speak(XString words) {
        /* null: games clear the queue with it (Healthy Life Journal) */
        done(SpeechListener.SPEECH_COMPLETE);
    }

    public void stop() {
    }

    public synchronized void setAttribute(int attr, int value) {
        if (attr == SPEECH_VOICE) {
            voice = value;
        } else if (attr == SPEECH_SPEED) {
            speed = value;
        } else {
            throw new IllegalArgumentException();
        }
    }

    public synchronized int getAttribute(int attr) {
        if (attr == SPEECH_VOICE) {
            return voice;
        }
        if (attr == SPEECH_SPEED) {
            return speed;
        }
        throw new IllegalArgumentException();
    }
}
