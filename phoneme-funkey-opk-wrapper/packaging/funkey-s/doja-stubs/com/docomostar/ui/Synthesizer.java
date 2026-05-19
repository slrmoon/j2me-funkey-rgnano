package com.docomostar.ui;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class Synthesizer {

    public int getAvailableMessageNum() {
    return 0;
    }

    public int getAvailableUCSDataNum() {
    return 0;
    }

    public int getAvailableUCSDataSize() {
    return 0;
    }

    public com.docomostar.ui.Synthesizer getSynthesizer(int p0) {
    return null;
    }

    public void play() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void sendMessage(byte[] p0, int p1, int p2) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setUCSData(byte[][] p0, byte[] p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void stop() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}