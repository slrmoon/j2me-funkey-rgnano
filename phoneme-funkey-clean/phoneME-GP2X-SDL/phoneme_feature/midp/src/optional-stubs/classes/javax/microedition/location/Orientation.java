package javax.microedition.location;

public class Orientation {
    private final float azimuth;
    private final boolean magnetic;
    private final float pitch;
    private final float roll;

    public Orientation(float azimuth, boolean isMagnetic, float pitch, float roll) {
        this.azimuth = azimuth;
        magnetic = isMagnetic;
        this.pitch = pitch;
        this.roll = roll;
    }

    public float getCompassAzimuth() {
        return azimuth;
    }

    public boolean isOrientationMagnetic() {
        return magnetic;
    }

    public float getPitch() {
        return pitch;
    }

    public float getRoll() {
        return roll;
    }

    /** FunKey: no compass */
    public static Orientation getOrientation() throws LocationException {
        throw new LocationException("orientation not available");
    }
}
