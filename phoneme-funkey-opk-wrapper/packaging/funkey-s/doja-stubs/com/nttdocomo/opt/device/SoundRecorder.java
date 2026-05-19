package com.nttdocomo.opt.device;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class SoundRecorder {

    public int getAttribute(int p0) {
    return 0;
    }

    public java.io.InputStream getInputStream() {
    return null;
    }

    public com.nttdocomo.opt.device.SoundRecorder getSoundRecorder() {
    return null;
    }

    public void record() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setAttribute(int p0, int p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void stop() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}