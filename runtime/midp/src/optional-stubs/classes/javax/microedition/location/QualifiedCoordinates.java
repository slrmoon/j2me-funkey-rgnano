package javax.microedition.location;

public class QualifiedCoordinates extends Coordinates {
    private float horizontalAccuracy;
    private float verticalAccuracy;

    public QualifiedCoordinates(double latitude, double longitude, float altitude,
            float horizontalAccuracy, float verticalAccuracy) {
        super(latitude, longitude, altitude);
        setHorizontalAccuracy(horizontalAccuracy);
        setVerticalAccuracy(verticalAccuracy);
    }

    public float getHorizontalAccuracy() {
        return horizontalAccuracy;
    }

    public float getVerticalAccuracy() {
        return verticalAccuracy;
    }

    public void setHorizontalAccuracy(float horizontalAccuracy) {
        if (horizontalAccuracy < 0) {
            throw new IllegalArgumentException();
        }
        this.horizontalAccuracy = horizontalAccuracy;
    }

    public void setVerticalAccuracy(float verticalAccuracy) {
        if (verticalAccuracy < 0) {
            throw new IllegalArgumentException();
        }
        this.verticalAccuracy = verticalAccuracy;
    }
}
