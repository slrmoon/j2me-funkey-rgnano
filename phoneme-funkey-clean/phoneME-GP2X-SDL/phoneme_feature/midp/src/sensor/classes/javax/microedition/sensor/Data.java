package javax.microedition.sensor;

public interface Data {
    public ChannelInfo getChannelInfo();
    public double[] getDoubleValues();
    public int[] getIntValues();
    public Object[] getObjectValues();
    public long getTimestamp(int index);
    public float getUncertainty(int index);
    public boolean isValid(int index);
}
