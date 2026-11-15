package com.nttdocomo.device.gesturereader;

/**
 * Motion input (the handset's camera or sensor follows how the phone is
 * moved). There is no such sensor here: the direction keys stand in for
 * tilting the phone and the select key (or 5) for shaking it.
 */
public abstract class GestureReader {
    boolean fkStarted;

    public GestureReader() {
    }

    public void start() {
        fkStarted = true;
    }

    public void stop() {
        fkStarted = false;
    }

    /** bit set of the DoJa keys held now (Display.KEY_*) */
    static int fkKeys() {
        return com.funkey.doja.DoJaRuntime.keypadState();
    }
}
