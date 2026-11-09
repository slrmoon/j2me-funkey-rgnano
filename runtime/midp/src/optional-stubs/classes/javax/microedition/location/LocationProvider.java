package javax.microedition.location;

/**
 * JSR 179. FunKey: no positioning: getInstance() throws LocationException
 * (every provider permanently out of service), getLastKnownLocation()
 * returns null.
 */
public abstract class LocationProvider {
    public static final int AVAILABLE = 1;
    public static final int TEMPORARILY_UNAVAILABLE = 2;
    public static final int OUT_OF_SERVICE = 3;

    protected LocationProvider() {
    }

    public static LocationProvider getInstance(Criteria criteria) throws LocationException {
        throw new LocationException("location not available");
    }

    public abstract Location getLocation(int timeout) throws LocationException, InterruptedException;

    public abstract void setLocationListener(LocationListener listener, int interval, int timeout,
            int maxAge);

    public abstract int getState();

    public abstract void reset();

    public static Location getLastKnownLocation() {
        return null;
    }

    public static void addProximityListener(ProximityListener listener, Coordinates coordinates,
            float proximityRadius) throws LocationException {
        if (listener == null || coordinates == null) {
            throw new NullPointerException();
        }
        throw new LocationException("location not available");
    }

    public static void removeProximityListener(ProximityListener listener) {
        if (listener == null) {
            throw new NullPointerException();
        }
    }
}
