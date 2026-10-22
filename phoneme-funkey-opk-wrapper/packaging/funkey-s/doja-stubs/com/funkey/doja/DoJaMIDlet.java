package com.funkey.doja;

import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

/**
 * MIDlet that boots a DoJa i-appli: the launcher passes the .jam contents
 * as system properties, the application class is created and started on
 * the DoJa event thread.
 */
public final class DoJaMIDlet extends MIDlet {
    private boolean started;

    protected void startApp() throws MIDletStateChangeException {
        if (started) {
            return;
        }
        started = true;
        DoJaRuntime.init(this);
        String className = DoJaRuntime.getJam("AppClass");
        if (className == null || className.length() == 0) {
            className = System.getProperty("doja.app.class");
        }
        if (className == null || className.length() == 0) {
            throw new MIDletStateChangeException("Missing AppClass");
        }
        System.out.println("DoJa: starting " + className);
        DoJaRuntime.startApplication(className);
    }

    protected void pauseApp() {
    }

    protected void destroyApp(boolean unconditional) {
        com.nttdocomo.ui.AudioPresenter.stopAll();
    }
}
