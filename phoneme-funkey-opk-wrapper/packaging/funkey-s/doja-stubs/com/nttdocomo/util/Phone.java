package com.nttdocomo.util;

import com.nttdocomo.lang.XString;

/** No phone calls on the handheld; fixed terminal properties. */
public final class Phone {
    public static final String TERMINAL_ID = "terminal-id";
    public static final String USER_ID = "user-id";
    public static final String TEL_AV = "tel-av:";
    public static final String UIM_VERSION = "uim-version";

    public Phone() {
    }

    public static void call(String destination) {
        com.funkey.doja.Stub.log("Phone.call");
    }

    public static void call(XString destination) {
        com.funkey.doja.Stub.log("Phone.call");
    }

    public static void call(String destination, XString subAddress) {
        com.funkey.doja.Stub.log("Phone.call");
    }

    public static String getProperty(String key) {
        if (key == null) {
            throw new NullPointerException("key");
        }
        if (TERMINAL_ID.equals(key)) {
            return "FUNKEY000000000";
        }
        if (USER_ID.equals(key)) {
            return "FUNKEY000000";
        }
        if (UIM_VERSION.equals(key)) {
            return "1";
        }
        return null;
    }
}
