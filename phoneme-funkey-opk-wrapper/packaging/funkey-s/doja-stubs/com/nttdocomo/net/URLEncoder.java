package com.nttdocomo.net;

public final class URLEncoder {
    private URLEncoder() {
    }

    public static String encode(String value) {
        if (value == null) {
            return "";
        }
        StringBuffer out = new StringBuffer();
        for (int i = 0; i < value.length(); i++) {
            char ch = value.charAt(i);
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.') {
                out.append(ch);
            } else if (ch == ' ') {
                out.append('+');
            } else {
                out.append('%');
                String hex = Integer.toHexString(ch).toUpperCase();
                if (hex.length() == 1) {
                    out.append('0');
                }
                out.append(hex);
            }
        }
        return out.toString();
    }
}
