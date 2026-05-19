package com.nttdocomo.device;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class DeviceException extends RuntimeException {

    public int getStatus() {
    return 0;
    }

    public void printStackTrace() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}