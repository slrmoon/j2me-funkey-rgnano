package com.nttdocomo.device.gesturereader;

/** Shaking the phone: here the select key or 5, held. */
public final class ShakeReader extends GestureReader {
    private static ShakeReader instance;

    public static synchronized ShakeReader getShakeReader(int id) {
        if (instance == null) {
            instance = new ShakeReader();
        }
        return instance;
    }

    /** how hard the phone is shaken, 0 = at rest */
    public int getShakeData() {
        if (!fkStarted) {
            return 0;
        }
        return (fkKeys() & (1 << 20 | 1 << 5)) != 0 ? 100 : 0;
    }

    protected ShakeReader() {
    }
}
