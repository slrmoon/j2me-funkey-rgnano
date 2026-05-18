package com.funkey.doja;

import java.io.IOException;

import javax.microedition.lcdui.Display;
import javax.microedition.midlet.MIDlet;

public final class DoJaRuntime {
    private static MIDlet midlet;

    private DoJaRuntime() {
    }

    public static void init(MIDlet m) {
        midlet = m;
    }

    public static MIDlet getMIDlet() {
        return midlet;
    }

    public static Display getMIDPDisplay() {
        return Display.getDisplay(midlet);
    }

    public static String getenv(String key) {
        try {
            String value = System.getProperty(key);
            if (value != null) {
                return value;
            }
            if ("DOJA_APP_CLASS".equals(key)) {
                return System.getProperty("doja.app.class");
            }
            if ("DOJA_APP_PARAM".equals(key)) {
                return System.getProperty("doja.app.param");
            }
            if ("DOJA_APP_VER".equals(key)) {
                return System.getProperty("doja.app.ver");
            }
            if ("DOJA_SOURCE_URL".equals(key)) {
                return System.getProperty("doja.source.url");
            }
            } catch (Throwable t) {
            return null;
        }
        return null;
    }

    public static byte[] readResource(String name) throws IOException {
        return new ResourceLoader().read(name);
    }
}
