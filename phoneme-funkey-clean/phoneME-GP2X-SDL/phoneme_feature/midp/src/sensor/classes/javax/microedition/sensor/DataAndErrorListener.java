package javax.microedition.sensor;

public interface DataAndErrorListener extends DataListener {
    public void errorReceived(SensorConnection sensor, int errorCode, long timestamp);
}
