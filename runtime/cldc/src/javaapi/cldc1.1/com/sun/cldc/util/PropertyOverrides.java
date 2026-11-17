/*
 * System properties the runtime decides after the VM has started (the
 * handset model a game expects, read from its JAR): System.getProperty
 * returns these before the -D / configuration values.
 */
package com.sun.cldc.util;

import java.util.Hashtable;

public final class PropertyOverrides {
    private static Hashtable values;

    private PropertyOverrides() {
    }

    public static synchronized void set(String key, String value) {
        if (values == null) {
            values = new Hashtable();
        }
        if (value == null) {
            values.remove(key);
        } else {
            values.put(key, value);
        }
    }

    public static synchronized String get(String key) {
        return values == null ? null : (String)values.get(key);
    }
}
