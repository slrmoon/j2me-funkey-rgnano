package com.nokia.mid.ui;

public final class DeviceControl {
    private static int lightLevel = 100;

    private DeviceControl() {
    }

    public static void setLights(int num, int level) {
        lightLevel = level;
    }

    public static void startVibra(int frequency, long duration) {
    }

    public static void stopVibra() {
    }

    public static void setLightOn(int num) {
        lightLevel = 100;
    }

    public static int getLightOn() {
        return lightLevel;
    }

    public static void flashLights(long duration) {
    }
}
