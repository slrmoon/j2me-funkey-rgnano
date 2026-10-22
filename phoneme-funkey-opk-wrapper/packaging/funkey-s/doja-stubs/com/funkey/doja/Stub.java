package com.funkey.doja;

import java.util.Hashtable;

/** Logs the first call of each unimplemented DoJa API method. */
public final class Stub {
    private static final Hashtable seen = new Hashtable();

    private Stub() {
    }

    public static void log(String name) {
        synchronized (seen) {
            if (seen.get(name) != null) {
                return;
            }
            seen.put(name, name);
        }
        System.out.println("DoJa: unimplemented " + name);
    }
}
