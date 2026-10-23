package com.nttdocomo.net;

/** x-www-form-urlencoded in the default encoding (Shift-JIS on i-mode). */
public final class URLEncoder {
    protected URLEncoder() {
    }

    public static String encode(String str) {
        if (str == null) {
            throw new NullPointerException("str");
        }
        byte[] b = str.getBytes();
        StringBuffer sb = new StringBuffer(b.length * 3);
        for (int i = 0; i < b.length; i++) {
            int c = b[i] & 0xFF;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
                    || c == '.' || c == '-' || c == '*' || c == '_') {
                sb.append((char) c);
            } else if (c == ' ') {
                sb.append('+');
            } else {
                sb.append('%').append("0123456789ABCDEF".charAt(c >> 4))
                        .append("0123456789ABCDEF".charAt(c & 15));
            }
        }
        return sb.toString();
    }
}
