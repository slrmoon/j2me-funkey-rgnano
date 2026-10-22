/* Samsung vibration: the FunKey has no motor. */
package com.samsung.util;

public class Vibration {
    public static void start(int duration, int strength) {
    }

    public static void stop() {
    }

    public static boolean isSupported() {
        return false;
    }
}
