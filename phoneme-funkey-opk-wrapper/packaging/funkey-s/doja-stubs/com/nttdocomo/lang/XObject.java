package com.nttdocomo.lang;

public abstract class XObject {
    private final Object value;

    protected XObject(Object v) {
        value = v;
    }

    protected XObject() {
        value = null;
    }

    protected final Object value() {
        return value;
    }

    public final boolean equals(Object obj) {
        if (!(obj instanceof XObject)) {
            return false;
        }
        Object o = ((XObject) obj).value;
        return value == null ? o == null : value.equals(o);
    }

    public final int hashCode() {
        return value == null ? 0 : value.hashCode();
    }

    public final String toString() {
        return value == null ? "" : value.toString();
    }
}
