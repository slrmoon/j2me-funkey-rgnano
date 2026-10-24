package com.nttdocomo.util;

/** RFC 2045 base64 without line breaks. */
public final class Base64 {
    private static final String ABC =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    protected Base64() {
    }

    public static byte[] decode(String value) {
        if (value == null) {
            throw new NullPointerException("value");
        }
        return decode(value.getBytes());
    }

    public static byte[] decode(byte[] value) {
        if (value == null) {
            throw new NullPointerException("value");
        }
        return decode(value, 0, value.length);
    }

    public static byte[] decode(byte[] value, int offset, int length) {
        if (value == null) {
            throw new NullPointerException("value");
        }
        if (offset < 0 || length < 0 || offset + length > value.length) {
            throw new ArrayIndexOutOfBoundsException();
        }
        byte[] out = new byte[length * 3 / 4 + 3];
        int n = 0;
        int acc = 0;
        int bits = 0;
        for (int i = offset; i < offset + length; i++) {
            int c = value[i];
            int v;
            if (c >= 'A' && c <= 'Z') {
                v = c - 'A';
            } else if (c >= 'a' && c <= 'z') {
                v = c - 'a' + 26;
            } else if (c >= '0' && c <= '9') {
                v = c - '0' + 52;
            } else if (c == '+' || c == '-') {
                v = 62;
            } else if (c == '/' || c == '_') {
                v = 63;
            } else if (c == '=') {
                break;
            } else {
                continue;   /* line breaks, spaces */
            }
            acc = (acc << 6) | v;
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                out[n++] = (byte) (acc >> bits);
            }
        }
        byte[] r = new byte[n];
        System.arraycopy(out, 0, r, 0, n);
        return r;
    }

    public static String encode(String value) {
        if (value == null) {
            throw new NullPointerException("value");
        }
        return encode(value.getBytes());
    }

    public static String encode(byte[] value) {
        if (value == null) {
            throw new NullPointerException("value");
        }
        return encode(value, 0, value.length);
    }

    public static String encode(byte[] value, int offset, int length) {
        if (value == null) {
            throw new NullPointerException("value");
        }
        if (offset < 0 || length < 0 || offset + length > value.length) {
            throw new ArrayIndexOutOfBoundsException();
        }
        StringBuffer sb = new StringBuffer((length + 2) / 3 * 4);
        int end = offset + length;
        for (int i = offset; i < end; i += 3) {
            int b0 = value[i] & 255;
            int b1 = i + 1 < end ? value[i + 1] & 255 : 0;
            int b2 = i + 2 < end ? value[i + 2] & 255 : 0;
            sb.append(ABC.charAt(b0 >> 2));
            sb.append(ABC.charAt(((b0 & 3) << 4) | (b1 >> 4)));
            sb.append(i + 1 < end ? ABC.charAt(((b1 & 15) << 2) | (b2 >> 6)) : '=');
            sb.append(i + 2 < end ? ABC.charAt(b2 & 63) : '=');
        }
        return sb.toString();
    }
}
