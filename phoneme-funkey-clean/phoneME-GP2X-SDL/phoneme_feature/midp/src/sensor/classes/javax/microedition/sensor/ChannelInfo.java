package javax.microedition.sensor;

public interface ChannelInfo {
    public static final int TYPE_DOUBLE = 1;
    public static final int TYPE_INT = 2;
    public static final int TYPE_OBJECT = 4;

    public float getAccuracy();
    public int getDataType();
    public MeasurementRange[] getMeasurementRanges();
    public String getName();
    public int getScale();
    public Unit getUnit();
}
