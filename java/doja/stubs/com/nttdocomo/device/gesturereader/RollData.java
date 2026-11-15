package com.nttdocomo.device.gesturereader;

public class RollData {
    public static final int QUALITY_RELIABLE = 0;
    public static final int QUALITY_LOW_DETAIL = 1;
    public static final int QUALITY_TOO_FAST = 2;
    public static final int QUALITY_SAME_VALUE = 3;

    float fkX;
    float fkY;
    float fkAccX;
    float fkAccY;

    public RollData() {
    }

    public int getQuality() {
        return QUALITY_RELIABLE;
    }

    /** movement at the moment of the reading: { x, y } */
    public float[] getImmediateMotion() {
        return new float[] { fkX, fkY };
    }

    /** movement since the previous reading: { x, y } */
    public float[] getAccumulatedMotion() {
        return new float[] { fkAccX, fkAccY };
    }
}
