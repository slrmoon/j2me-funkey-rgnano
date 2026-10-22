package com.nttdocomo.lang;

/** Immutable string that the application cannot read back as String. */
public final class XString extends XObject {
    public XString(String s) {
        super(check(s));
    }

    private static String check(String s) {
        if (s == null) {
            throw new NullPointerException("value");
        }
        return s;
    }

    public int length() {
        return toString().length();
    }

    public XString concat(XString other) {
        return new XString(toString() + other.toString());
    }

    public XString concat(String other) {
        return new XString(toString() + other);
    }
}
