package javax.microedition.location;

public class Location {
    public static final int MTE_SATELLITE = 0x00000001;
    public static final int MTE_TIMEDIFFERENCE = 0x00000002;
    public static final int MTE_TIMEOFARRIVAL = 0x00000004;
    public static final int MTE_CELLID = 0x00000008;
    public static final int MTE_SHORTRANGE = 0x00000010;
    public static final int MTE_ANGLEOFARRIVAL = 0x00000020;
    public static final int MTY_TERMINALBASED = 0x00010000;
    public static final int MTY_NETWORKBASED = 0x00020000;
    public static final int MTA_ASSISTED = 0x00040000;
    public static final int MTA_UNASSISTED = 0x00080000;

    protected Location() {
    }

    public boolean isValid() {
        return false;
    }

    public long getTimestamp() {
        return 0;
    }

    public QualifiedCoordinates getQualifiedCoordinates() {
        return null;
    }

    public float getSpeed() {
        return Float.NaN;
    }

    public float getCourse() {
        return Float.NaN;
    }

    public int getLocationMethod() {
        return 0;
    }

    public AddressInfo getAddressInfo() {
        return null;
    }

    public String getExtraInfo(String mimetype) {
        return null;
    }
}
