package com.nttdocomo.device.gesturereader;

/** Tilt of the phone: here the direction keys (also 4 6 2 8). */
public final class RollReader extends GestureReader {
    /* what one reading says while a key is held; games add the readings
     * up and stop at about a hundred */
    private static final float STEP = 3.0f;
    private static RollReader instance;

    public static synchronized RollReader getRollReader(int id) {
        if (instance == null) {
            instance = new RollReader();
        }
        return instance;
    }

    public void recenter() {
    }

    public RollData getRollData() {
        RollData d = new RollData();
        if (!fkStarted) {
            return d;
        }
        int k = fkKeys();
        float x = 0;
        float y = 0;
        if ((k & (1 << 16 | 1 << 4)) != 0) {
            x -= STEP;
        }
        if ((k & (1 << 18 | 1 << 6)) != 0) {
            x += STEP;
        }
        if ((k & (1 << 17 | 1 << 2)) != 0) {
            y -= STEP;
        }
        if ((k & (1 << 19 | 1 << 8)) != 0) {
            y += STEP;
        }
        d.fkX = x;
        d.fkY = y;
        d.fkAccX = x;
        d.fkAccY = y;
        return d;
    }

    protected RollReader() {
    }
}
