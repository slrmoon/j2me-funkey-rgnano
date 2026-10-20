/* Siemens game sound: a single beep. */
package com.siemens.mp.game;

import javax.microedition.media.Manager;

public class Sound {
    private Sound() {
    }

    /** freq in Hz, time in ms. */
    public static void playTone(int freq, int time) {
        if (freq <= 0 || time <= 0) return;
        try {
            Manager.playTone(MelodyComposer.freqToNote(freq), time, 100);
        } catch (Exception e) {
            /* ignore */
        }
    }
}
