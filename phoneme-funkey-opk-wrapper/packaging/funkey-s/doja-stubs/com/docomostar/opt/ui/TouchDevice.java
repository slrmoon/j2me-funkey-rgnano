package com.docomostar.opt.ui;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class TouchDevice extends Object {

    public int getX() {
    return 0;
    }

    public int getY() {
    return 0;
    }

    public boolean isAvailable() {
    return false;
    }

    public boolean isEnabled() {
    return false;
    }

    public void setEnabled(boolean p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}