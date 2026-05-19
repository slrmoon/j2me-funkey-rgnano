package com.docomostar.ui;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class UIException extends RuntimeException {

    public java.lang.String getMessage() {
    return null;
    }

    public int getStatus() {
    return 0;
    }

    public void printStackTrace() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public java.lang.String toString() {
    return null;
    }

}