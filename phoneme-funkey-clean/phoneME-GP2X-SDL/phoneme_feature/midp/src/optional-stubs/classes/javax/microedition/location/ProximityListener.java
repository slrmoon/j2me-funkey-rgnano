package javax.microedition.location;

public interface ProximityListener {
    void proximityEvent(Coordinates coordinates, Location location);

    void monitoringStateChanged(boolean isMonitoringActive);
}
