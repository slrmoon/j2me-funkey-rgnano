package com.docomostar.device;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class RemoteDevice {

    public com.docomostar.io.BTConnection accept(int p0) {
    return null;
    }

    public com.docomostar.io.BTConnection connect(int p0) {
    return null;
    }

    public void dispose() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public java.lang.String getAddress() {
    return null;
    }

    public java.lang.String getDeviceClass() {
    return null;
    }

    public java.lang.String getDeviceName() {
    return null;
    }

    public void interruptAcceptance() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public boolean isAvailable(int p0) {
    return false;
    }

}