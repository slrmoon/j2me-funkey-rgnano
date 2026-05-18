package com.funkey.doja;

import com.nttdocomo.ui.IApplication;

import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

public final class DoJaMIDlet extends MIDlet {
    private IApplication app;
    private boolean started;

    protected void startApp() throws MIDletStateChangeException {
        if (started) {
            return;
        }
        started = true;
        DoJaRuntime.init(this);
        try {
            String className = System.getProperty("doja.app.class");
            if (className == null || className.length() == 0) {
                className = DoJaRuntime.getenv("DOJA_APP_CLASS");
            }
            if (className == null || className.length() == 0) {
                throw new ClassNotFoundException("Missing DOJA_APP_CLASS");
            }
            app = (IApplication)Class.forName(className).newInstance();
            IApplication.setCurrentApp(app);
            app.start();
        } catch (Throwable t) {
            t.printStackTrace();
            throw new MIDletStateChangeException(t.toString());
        }
    }

    protected void pauseApp() {
        if (app != null) {
            try {
                app.pause();
            } catch (Throwable t) {
                t.printStackTrace();
            }
        }
    }

    protected void destroyApp(boolean unconditional) {
        if (app != null) {
            try {
                app.destroy();
            } catch (Throwable t) {
                t.printStackTrace();
            }
        }
    }
}
