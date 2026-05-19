package com.docomostar;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class StarApplication extends Object {

    public StarApplication() {
    super();
    }

    public void activated(int p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void addEventListener(int p0, com.docomostar.StarEventListener p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public int getAppFaceState() {
    return 0;
    }

    public int getAppState() {
    return 0;
    }

    public com.docomostar.StarApplicationManager getStarApplicationManager() {
    return null;
    }

    public com.docomostar.StarApplication getThisStarApplication() {
    return null;
    }

    public void terminate() {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

}