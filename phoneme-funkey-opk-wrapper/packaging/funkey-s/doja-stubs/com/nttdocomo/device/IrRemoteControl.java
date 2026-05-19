package com.nttdocomo.device;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class IrRemoteControl {

    public IrRemoteControl() {
    super();
    }

    public com.nttdocomo.device.IrRemoteControl getIrRemoteControl() {
    return null;
    }

    public void send(int p0, com.nttdocomo.device.IrRemoteControlFrame[] p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void send(int p0, com.nttdocomo.device.IrRemoteControlFrame[] p1, int p2) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setCarrier(int p0, int p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setCode0(int p0, int p1, int p2) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setCode1(int p0, int p1, int p2) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void stop() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}