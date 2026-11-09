package javax.bluetooth;

public class DeviceClass {
    private final int record;

    public DeviceClass(int record) {
        if ((record & 0xff000000) != 0) {
            throw new IllegalArgumentException();
        }
        this.record = record;
    }

    public int getServiceClasses() {
        return record & 0xffe000;
    }

    public int getMajorDeviceClass() {
        return record & 0x1f00;
    }

    public int getMinorDeviceClass() {
        return record & 0xfc;
    }
}
