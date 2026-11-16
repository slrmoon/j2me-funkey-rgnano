package com.nttdocomo.net;

/** x-www-form-urlencoded back to text, in the default encoding (Shift-JIS on i-mode). */
public final class URLDecoder {
    public URLDecoder() {
    }

    private static int hex(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    public static String decode(String str) {
        if (str == null) {
            throw new NullPointerException("str");
        }
        int n = str.length();
        byte[] out = new byte[n * 3];
        int k = 0;
        for (int i = 0; i < n; i++) {
            char c = str.charAt(i);
            if (c == '+') {
                out[k++] = ' ';
            } else if (c == '%' && i + 2 < n && hex(str.charAt(i + 1)) >= 0
                    && hex(str.charAt(i + 2)) >= 0) {
                out[k++] = (byte) (hex(str.charAt(i + 1)) * 16 + hex(str.charAt(i + 2)));
                i += 2;
            } else if (c < 0x80) {
                out[k++] = (byte) c;
            } else {
                byte[] b = String.valueOf(c).getBytes();
                for (int j = 0; j < b.length; j++) {
                    out[k++] = b[j];
                }
            }
        }
        return new String(out, 0, k);
    }
}
