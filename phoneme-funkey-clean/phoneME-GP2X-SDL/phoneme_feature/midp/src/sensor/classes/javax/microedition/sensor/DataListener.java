package javax.microedition.sensor;

public interface DataListener {
    public void dataReceived(SensorConnection sensor, Data[] data, boolean isDataLost);
}
