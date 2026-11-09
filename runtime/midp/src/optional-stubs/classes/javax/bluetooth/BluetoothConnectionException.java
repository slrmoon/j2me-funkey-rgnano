package javax.bluetooth;

import java.io.IOException;

public class BluetoothConnectionException extends IOException {
    public static final int UNKNOWN_PSM = 1;
    public static final int SECURITY_BLOCK = 2;
    public static final int NO_RESOURCES = 3;
    public static final int FAILED_NOINFO = 4;
    public static final int TIMEOUT = 5;
    public static final int UNACCEPTABLE_PARAMS = 6;

    private final int status;

    public BluetoothConnectionException(int error) {
        this(error, null);
    }

    public BluetoothConnectionException(int error, String msg) {
        super(msg);
        if (error < UNKNOWN_PSM || error > UNACCEPTABLE_PARAMS) {
            throw new IllegalArgumentException();
        }
        status = error;
    }

    public int getStatus() {
        return status;
    }
}
