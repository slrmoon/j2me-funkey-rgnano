package com.docomostar.device;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class BluetoothException extends RuntimeException {

    public java.lang.String getMessage() {
    return null;
    }

    public int getStatus() {
    return 0;
    }

    public void printStackTrace() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}