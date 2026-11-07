package javax.bluetooth;

/** FunKey: never handed out (no local device). */
public class DiscoveryAgent {
    public static final int NOT_DISCOVERABLE = 0;
    public static final int GIAC = 0x9E8B33;
    public static final int LIAC = 0x9E8B00;
    public static final int CACHED = 0x00;
    public static final int PREKNOWN = 0x01;

    DiscoveryAgent() {
    }

    public RemoteDevice[] retrieveDevices(int option) {
        return null;
    }

    public boolean startInquiry(int accessCode, DiscoveryListener listener)
            throws BluetoothStateException {
        throw new BluetoothStateException("Bluetooth not available");
    }

    public boolean cancelInquiry(DiscoveryListener listener) {
        return false;
    }

    public int searchServices(int[] attrSet, UUID[] uuidSet, RemoteDevice btDev,
            DiscoveryListener discListener) throws BluetoothStateException {
        throw new BluetoothStateException("Bluetooth not available");
    }

    public boolean cancelServiceSearch(int transID) {
        return false;
    }

    public String selectService(UUID uuid, int security, boolean master)
            throws BluetoothStateException {
        throw new BluetoothStateException("Bluetooth not available");
    }
}
