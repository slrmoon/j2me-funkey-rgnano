package com.nttdocomo.device.location;

/** Readings of the acceleration sensor, oldest first. */
public class AccelerationData {
    int[] fkX;
    int[] fkY;
    int[] fkZ;
    int[] fkRoll;
    int[] fkPitch;
    int[] fkOrient;

    AccelerationData(int[][] samples, int n) {
        fkX = new int[n];
        fkY = new int[n];
        fkZ = new int[n];
        fkRoll = new int[n];
        fkPitch = new int[n];
        fkOrient = new int[n];
        for (int i = 0; i < n; i++) {
            fkX[i] = samples[i][0];
            fkY[i] = samples[i][1];
            fkZ[i] = samples[i][2];
            fkRoll[i] = samples[i][3];
            fkPitch[i] = samples[i][4];
            fkOrient[i] = samples[i][5];
        }
    }

    protected AccelerationData() {
    }

    public int[] getAccelerationX() {
        return fkX;
    }

    public int[] getAccelerationY() {
        return fkY;
    }

    public int[] getAccelerationZ() {
        return fkZ;
    }

    public int[] getRoll() {
        return fkRoll;
    }

    public int[] getPitch() {
        return fkPitch;
    }

    public int[] getScreenOrientation() {
        return fkOrient;
    }

    public int size() {
        return fkX == null ? 0 : fkX.length;
    }
}
