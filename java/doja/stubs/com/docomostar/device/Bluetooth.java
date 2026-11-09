package com.docomostar.device;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class Bluetooth {

    public int getDiscoveredDevice() {
    return 0;
    }

    public int getInquiryTimeout() {
    return 0;
    }

    public com.docomostar.device.Bluetooth getInstance() {
    return null;
    }

    public boolean isConnectable(int p0) {
    return false;
    }

    public com.docomostar.device.RemoteDevice scan() {
    return null;
    }

    public com.docomostar.device.RemoteDevice searchAndSelectDevice() {
    return null;
    }

    public com.docomostar.device.RemoteDevice selectDevice() {
    return null;
    }

    public void setDetachmentMode(boolean p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setInquiryTimeout(int p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public com.docomostar.device.RemoteDevice startPairingByOOB(int p0, int p1, byte[] p2) {
    return null;
    }

    public void turnOff() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}