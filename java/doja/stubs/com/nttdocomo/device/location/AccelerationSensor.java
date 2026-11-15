package com.nttdocomo.device.location;

/**
 * The acceleration sensor, worked by the keys: the direction keys (or
 * 4 6 2 8) tilt the phone, select / 5 shakes it. Values in mG
 * (the phone lying still: x 0, y 0, z -1000), roll and pitch in degrees.
 */
public class AccelerationSensor {
    public static final int ACCELERATION_X = 1;
    public static final int ACCELERATION_Y = 2;
    public static final int ACCELERATION_Z = 3;
    public static final int ROLL = 4;
    public static final int PITCH = 5;
    public static final int SCREEN_ORIENTATION = 6;
    public static final int DOUBLE_TAP_LEFT = 0;
    public static final int DOUBLE_TAP_RIGHT = 1;
    public static final int DOUBLE_TAP_FRONT = 2;
    public static final int DOUBLE_TAP_BACK = 3;
    public static final int EVENT_SCREEN_ORIENTATION = 1;
    public static final int EVENT_DOUBLE_TAP = 2;

    /* a held key tilts the phone by this much (about 40 degrees) */
    private static final int TILT = 640;
    private static final int TILT_DEG = 40;
    private static final int MAX = 64;
    private static AccelerationSensor instance;

    private final int[][] buf = new int[MAX][6];
    private int count;
    private int interval;
    private Thread sampler;
    private int shake;

    public static synchronized AccelerationSensor getAccelerationSensor() {
        if (instance == null) {
            instance = new AccelerationSensor();
        }
        return instance;
    }

    protected AccelerationSensor() {
    }

    public int[] getAvailableData() {
        return new int[] { ACCELERATION_X, ACCELERATION_Y, ACCELERATION_Z, ROLL, PITCH, SCREEN_ORIENTATION };
    }

    public int getMinDataValue(int type) {
        switch (type) {
        case ACCELERATION_X:
        case ACCELERATION_Y:
        case ACCELERATION_Z: return -4000;
        case ROLL:
        case PITCH: return -180;
        case SCREEN_ORIENTATION: return 0;
        default: throw new IllegalArgumentException("type");
        }
    }

    public int getMaxDataValue(int type) {
        switch (type) {
        case ACCELERATION_X:
        case ACCELERATION_Y:
        case ACCELERATION_Z: return 4000;
        case ROLL: return 179;
        case PITCH: return 180;
        case SCREEN_ORIENTATION: return 270;
        default: throw new IllegalArgumentException("type");
        }
    }

    /** one reading from the keys held now */
    private void sample(int[] s) {
        int k = com.funkey.doja.DoJaRuntime.keypadState();
        int x = 0;
        int y = 0;
        if ((k & (1 << 16 | 1 << 4)) != 0) {
            x -= 1;
        }
        if ((k & (1 << 18 | 1 << 6)) != 0) {
            x += 1;
        }
        if ((k & (1 << 17 | 1 << 2)) != 0) {
            y += 1;
        }
        if ((k & (1 << 19 | 1 << 8)) != 0) {
            y -= 1;
        }
        int z = -1000;
        if ((k & (1 << 20 | 1 << 5)) != 0) {
            /* shaking: the reading swings back and forth */
            shake++;
            z = (shake & 1) == 0 ? 1500 : -2500;
        }
        if (TURNED != 0) {
            /* the phone held sideways (web page ?rotate): gravity along
             * its x axis, the keys tilt it as seen on the turned screen */
            int sx = TURNED * y;
            int sy = -TURNED * x;
            s[0] = (z == -1000 ? TURNED * 1000 : z) + sx * TILT / 2;
            s[1] = sy * TILT;
            s[2] = sx * TILT;
            s[3] = sx * TILT_DEG;
            s[4] = sy * TILT_DEG;
            s[5] = TURNED > 0 ? 90 : 270;
            return;
        }
        s[0] = x * TILT;
        s[1] = y * TILT;
        s[2] = z;
        s[3] = x * TILT_DEG;
        s[4] = y * TILT_DEG;
        s[5] = 0;
    }

    /* -Ddoja.screen.rotate=left|right: the player turned the picture */
    private static final int TURNED = "left".equals(System.getProperty("doja.screen.rotate")) ? -1
            : "right".equals(System.getProperty("doja.screen.rotate")) ? 1 : 0;

    public AccelerationData getCurrentData() {
        int[][] one = new int[1][6];
        sample(one[0]);
        return new AccelerationData(one, 1);
    }

    public synchronized void start(int interval) {
        if (interval <= 0) {
            throw new IllegalArgumentException("interval");
        }
        this.interval = interval < 20 ? 20 : interval;
        count = 0;
        if (sampler == null) {
            sampler = new Thread() {
                public void run() {
                    while (true) {
                        int ms;
                        synchronized (AccelerationSensor.this) {
                            if (sampler != this) {
                                return;
                            }
                            if (count == MAX) {
                                System.arraycopy(buf, 1, buf, 0, MAX - 1);
                                buf[MAX - 1] = new int[6];
                                count--;
                            }
                            sample(buf[count++]);
                            ms = AccelerationSensor.this.interval;
                        }
                        try {
                            Thread.sleep(ms);
                        } catch (InterruptedException e) {
                            return;
                        }
                    }
                }
            };
            sampler.start();
        }
    }

    public synchronized void stop() {
        sampler = null;
    }

    public int getIntervalResolution() {
        return 20;
    }

    public int getMaxDataSize() {
        return MAX;
    }

    public synchronized int getDataSize() {
        return count;
    }

    public synchronized AccelerationData getData() {
        AccelerationData d = new AccelerationData(buf, count);
        count = 0;
        return d;
    }

    public synchronized AccelerationData peekLatestData() {
        if (count == 0) {
            return getCurrentData();
        }
        int[][] one = { buf[count - 1] };
        return new AccelerationData(one, 1);
    }

    public synchronized void disposeData() {
        count = 0;
    }

    public int[] getAvailableEvent() {
        return null;
    }

    public void startEventDetection(int event) {
        throw new IllegalArgumentException("event");
    }

    public void stopEventDetection() {
    }

    public void setEventListener(AccelerationEventListener listener) {
    }
}
