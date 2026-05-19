package com.nttdocomo.device;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class Camera {

    public void disposeImages() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public int[] getAvailableFocusModes() {
    return null;
    }

    public int[][] getAvailableFrameSizes() {
    return null;
    }

    public int[][] getAvailablePictureSizes() {
    return null;
    }

    public com.nttdocomo.device.Camera getCamera(int p0) {
    return null;
    }

    public com.nttdocomo.ui.MediaImage getImage(int p0) {
    return null;
    }

    public long getImageLength(int p0) {
    return 0;
    }

    public java.io.InputStream getInputStream(int p0) {
    return null;
    }

    public int getNumberOfImages() {
    return 0;
    }

    public boolean isAvailable(int p0) {
    return false;
    }

    public void setAttribute(int p0, int p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setFocusMode(int p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setFrameImage(com.nttdocomo.ui.MediaImage p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setImageSize(int p0, int p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void takePicture() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}