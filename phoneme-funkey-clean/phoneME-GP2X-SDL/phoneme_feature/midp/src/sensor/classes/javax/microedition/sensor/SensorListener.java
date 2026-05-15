package javax.microedition.sensor;

public interface SensorListener {
    public void sensorAvailable(SensorInfo info);
    public void sensorUnavailable(SensorInfo info);
}
