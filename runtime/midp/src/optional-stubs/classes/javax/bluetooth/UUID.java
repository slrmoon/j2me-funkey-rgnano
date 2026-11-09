package javax.bluetooth;

/** 128-bit Bluetooth UUID (short forms on the Bluetooth base UUID). */
public class UUID {
    private static final String BASE = "0000000000001000800000805F9B34FB";
    private final String hex;   /* 32 upper-case hex digits */

    public UUID(long uuidValue) {
        if (uuidValue < 0 || uuidValue > 0xffffffffL) {
            throw new IllegalArgumentException();
        }
        String h = Long.toString(uuidValue, 16).toUpperCase();
        while (h.length() < 8) {
            h = "0" + h;
        }
        hex = h + BASE.substring(8);
    }

    public UUID(String uuidValue, boolean shortUUID) {
        if (uuidValue == null) {
            throw new NullPointerException();
        }
        int n = uuidValue.length();
        if (n == 0 || n > 32 || (shortUUID && n > 8)) {
            throw new IllegalArgumentException();
        }
        for (int i = 0; i < n; i++) {
            if (Character.digit(uuidValue.charAt(i), 16) < 0) {
                throw new NumberFormatException();
            }
        }
        String h = uuidValue.toUpperCase();
        if (shortUUID) {
            while (h.length() < 8) {
                h = "0" + h;
            }
            hex = h + BASE.substring(8);
        } else {
            while (h.length() < 32) {
                h = "0" + h;
            }
            hex = h;
        }
    }

    public String toString() {
        int i = 0;
        while (i < 31 && hex.charAt(i) == '0') {
            i++;
        }
        return hex.substring(i);
    }

    public boolean equals(Object value) {
        return value instanceof UUID && ((UUID) value).hex.equals(hex);
    }

    public int hashCode() {
        return hex.hashCode();
    }
}
