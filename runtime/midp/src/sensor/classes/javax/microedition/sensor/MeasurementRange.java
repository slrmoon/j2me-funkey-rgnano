package javax.microedition.sensor;

public class MeasurementRange {
    private final double smallest;
    private final double largest;
    private final double resolution;

    public MeasurementRange(double smallest, double largest, double resolution) {
        if (smallest > largest || resolution < 0) {
            throw new IllegalArgumentException();
        }

        this.smallest = smallest;
        this.largest = largest;
        this.resolution = resolution;
    }

    public double getLargestValue() {
        return largest;
    }

    public double getResolution() {
        return resolution;
    }

    public double getSmallestValue() {
        return smallest;
    }
}
