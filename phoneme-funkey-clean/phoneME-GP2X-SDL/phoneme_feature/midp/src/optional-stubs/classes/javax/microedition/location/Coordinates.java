package javax.microedition.location;

/** WGS84 point; distance and azimuth on a sphere (CLDC has no atan2/asin). */
public class Coordinates {
    public static final int DD_MM_SS = 1;
    public static final int DD_MM = 2;

    private static final double EARTH_RADIUS = 6371008.8;

    private double latitude;
    private double longitude;
    private float altitude;

    public Coordinates(double latitude, double longitude, float altitude) {
        setLatitude(latitude);
        setLongitude(longitude);
        this.altitude = altitude;
    }

    public double getLatitude() {
        return latitude;
    }

    public double getLongitude() {
        return longitude;
    }

    public float getAltitude() {
        return altitude;
    }

    public void setAltitude(float altitude) {
        this.altitude = altitude;
    }

    public void setLatitude(double latitude) {
        if (Double.isNaN(latitude) || latitude < -90.0 || latitude > 90.0) {
            throw new IllegalArgumentException();
        }
        this.latitude = latitude;
    }

    public void setLongitude(double longitude) {
        if (Double.isNaN(longitude) || longitude < -180.0 || longitude >= 180.0) {
            throw new IllegalArgumentException();
        }
        this.longitude = longitude;
    }

    /** "DD:MM.mm" or "DD:MM:SS.ss" (optionally signed) to degrees */
    public static double convert(String coordinate) {
        if (coordinate == null) {
            throw new NullPointerException();
        }
        try {
            String s = coordinate.trim();
            boolean neg = s.startsWith("-");
            if (neg) {
                s = s.substring(1);
            }
            int a = s.indexOf(':');
            if (a <= 0) {
                throw new IllegalArgumentException();
            }
            double deg = Integer.parseInt(s.substring(0, a));
            String rest = s.substring(a + 1);
            int b = rest.indexOf(':');
            double min;
            double sec = 0.0;
            if (b < 0) {
                min = Double.parseDouble(rest);
            } else {
                min = Integer.parseInt(rest.substring(0, b));
                sec = Double.parseDouble(rest.substring(b + 1));
            }
            if (deg > 180 || min < 0 || min >= 60 || sec < 0 || sec >= 60) {
                throw new IllegalArgumentException();
            }
            double v = deg + min / 60.0 + sec / 3600.0;
            return neg ? -v : v;
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException();
        }
    }

    public static String convert(double coordinate, int outputType) {
        if (Double.isNaN(coordinate) || coordinate < -180.0 || coordinate >= 180.0
                || (outputType != DD_MM && outputType != DD_MM_SS)) {
            throw new IllegalArgumentException();
        }
        boolean neg = coordinate < 0;
        double v = neg ? -coordinate : coordinate;
        int deg = (int) v;
        double minutes = (v - deg) * 60.0;
        StringBuffer sb = new StringBuffer();
        if (neg) {
            sb.append('-');
        }
        sb.append(deg).append(':');
        if (outputType == DD_MM) {
            long m = (long) (minutes * 100000.0 + 0.5);
            appendFixed(sb, m, 5);
        } else {
            int min = (int) minutes;
            double secs = (minutes - min) * 60.0;
            if (min < 10) {
                sb.append('0');
            }
            sb.append(min).append(':');
            long s = (long) (secs * 1000.0 + 0.5);
            appendFixed(sb, s, 3);
        }
        return sb.toString();
    }

    private static void appendFixed(StringBuffer sb, long v, int decimals) {
        long scale = 1;
        for (int i = 0; i < decimals; i++) {
            scale *= 10;
        }
        long whole = v / scale;
        if (whole < 10) {
            sb.append('0');
        }
        sb.append(whole).append('.');
        String f = Long.toString(v % scale);
        for (int i = f.length(); i < decimals; i++) {
            sb.append('0');
        }
        sb.append(f);
    }

    private static double atan(double x) {
        /* range reduction to |x| <= 0.268 (tan 15 deg), then the series */
        boolean neg = x < 0;
        if (neg) {
            x = -x;
        }
        boolean inv = x > 1.0;
        if (inv) {
            x = 1.0 / x;
        }
        boolean shift = x > 0.2679491924311227;
        if (shift) {
            x = (x * 1.7320508075688772 - 1.0) / (x + 1.7320508075688772);
        }
        double x2 = x * x;
        double r = x * (1.0 - x2 * (1.0 / 3 - x2 * (1.0 / 5 - x2 * (1.0 / 7 - x2 * (1.0 / 9
                - x2 * (1.0 / 11 - x2 / 13))))));
        if (shift) {
            r += Math.PI / 6;
        }
        if (inv) {
            r = Math.PI / 2 - r;
        }
        return neg ? -r : r;
    }

    private static double atan2(double y, double x) {
        if (x > 0) {
            return atan(y / x);
        }
        if (x < 0) {
            return y >= 0 ? atan(y / x) + Math.PI : atan(y / x) - Math.PI;
        }
        return y > 0 ? Math.PI / 2 : y < 0 ? -Math.PI / 2 : 0.0;
    }

    /** great-circle distance in meters (haversine) */
    public float distance(Coordinates to) {
        if (to == null) {
            throw new NullPointerException();
        }
        double p1 = Math.toRadians(latitude);
        double p2 = Math.toRadians(to.latitude);
        double dp = p2 - p1;
        double dl = Math.toRadians(to.longitude - longitude);
        double s1 = Math.sin(dp / 2);
        double s2 = Math.sin(dl / 2);
        double a = s1 * s1 + Math.cos(p1) * Math.cos(p2) * s2 * s2;
        return (float) (2 * EARTH_RADIUS * atan2(Math.sqrt(a), Math.sqrt(1 - a)));
    }

    /** initial bearing in degrees 0..360 (0 = north) */
    public float azimuthTo(Coordinates to) {
        if (to == null) {
            throw new NullPointerException();
        }
        double p1 = Math.toRadians(latitude);
        double p2 = Math.toRadians(to.latitude);
        double dl = Math.toRadians(to.longitude - longitude);
        double y = Math.sin(dl) * Math.cos(p2);
        double x = Math.cos(p1) * Math.sin(p2) - Math.sin(p1) * Math.cos(p2) * Math.cos(dl);
        double deg = Math.toDegrees(atan2(y, x));
        if (deg < 0) {
            deg += 360.0;
        }
        return (float) deg;
    }
}
