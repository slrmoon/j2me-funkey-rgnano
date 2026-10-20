/*
 * Siemens game melody, built by MelodyComposer. Plays as a JSR-135 tone
 * sequence; only one melody plays at a time.
 */
package com.siemens.mp.game;

import javax.microedition.media.Manager;
import javax.microedition.media.Player;
import javax.microedition.media.control.ToneControl;

public class Melody extends com.siemens.mp.misc.NativeMem {
    private static Player current;

    private final byte[] sequence;
    private final boolean forever;

    Melody(byte[] sequence, boolean forever) {
        this.sequence = sequence;
        this.forever = forever;
    }

    public void play() {
        synchronized (Melody.class) {
            stop();
            if (sequence == null) return;
            try {
                Player p = Manager.createPlayer(Manager.TONE_DEVICE_LOCATOR);
                p.realize();
                ((ToneControl) p.getControl("ToneControl")).setSequence(sequence);
                if (forever) p.setLoopCount(-1);
                p.start();
                current = p;
            } catch (Exception e) {
                current = null;
            }
        }
    }

    public static void stop() {
        synchronized (Melody.class) {
            if (current != null) {
                current.close();
                current = null;
            }
        }
    }
}
