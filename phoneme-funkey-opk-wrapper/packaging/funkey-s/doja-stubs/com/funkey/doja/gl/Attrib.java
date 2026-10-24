package com.funkey.doja.gl;

/** A vertex attribute array: values read by vertex index and component. */
public final class Attrib {
    public static final int BYTE = 0x1400;
    public static final int UNSIGNED_BYTE = 0x1401;
    public static final int SHORT = 0x1402;
    public static final int UNSIGNED_SHORT = 0x1403;
    public static final int FLOAT = 0x1406;
    public static final int FIXED = 0x140C;

    public int size;
    public int type;
    /** distance between vertices in array elements */
    public int step;
    public int offset;
    public float[] f;
    public short[] s;
    public byte[] b;
    public int[] i;
    /** bytes interpreted as other element types (interleaved byte buffers) */
    public boolean bytesAsType;

    public void set(int size, int type, int strideBytes, Object data, int offset) {
        this.size = size;
        this.type = type;
        f = null;
        s = null;
        b = null;
        i = null;
        bytesAsType = false;
        int elem = elemBytes(type);
        if (data instanceof float[]) {
            f = (float[]) data;
            elem = 4;
        } else if (data instanceof short[]) {
            s = (short[]) data;
            elem = 2;
        } else if (data instanceof int[]) {
            i = (int[]) data;
            elem = 4;
        } else if (data instanceof byte[]) {
            b = (byte[]) data;
            bytesAsType = elemBytes(type) > 1;
            elem = bytesAsType ? 1 : 1;
        }
        if (bytesAsType) {
            step = strideBytes > 0 ? strideBytes : size * elemBytes(type);
        } else {
            step = strideBytes > 0 ? strideBytes / elem : size;
        }
        this.offset = offset;
    }

    static int elemBytes(int type) {
        switch (type) {
        case BYTE:
        case UNSIGNED_BYTE:
            return 1;
        case SHORT:
        case UNSIGNED_SHORT:
            return 2;
        default:
            return 4;
        }
    }

    public boolean valid() {
        return f != null || s != null || b != null || i != null;
    }

    /** component k of vertex v; normalized: integer colors become 0..1 */
    public float get(int v, int k, boolean normalized) {
        if (bytesAsType) {
            int p = offset + v * step + k * elemBytes(type);
            if (p < 0 || p + elemBytes(type) > b.length) {
                return 0f;
            }
            switch (type) {
            case SHORT:
            case UNSIGNED_SHORT: {
                int x = (b[p] & 255) | (b[p + 1] << 8);
                if (type == UNSIGNED_SHORT) {
                    x &= 0xffff;
                } else {
                    x = (short) x;
                }
                return normalized ? x / (type == SHORT ? 32767f : 65535f) : x;
            }
            case FLOAT: {
                int bits = (b[p] & 255) | ((b[p + 1] & 255) << 8) | ((b[p + 2] & 255) << 16) | (b[p + 3] << 24);
                return Float.intBitsToFloat(bits);
            }
            default: {
                int bits = (b[p] & 255) | ((b[p + 1] & 255) << 8) | ((b[p + 2] & 255) << 16) | (b[p + 3] << 24);
                return bits / 65536f;
            }
            }
        }
        int p = offset + v * step + k;
        if (f != null) {
            return p < f.length && p >= 0 ? f[p] : 0f;
        }
        if (s != null) {
            if (p >= s.length || p < 0) {
                return 0f;
            }
            int x = type == UNSIGNED_SHORT ? s[p] & 0xffff : s[p];
            return normalized ? x / (type == UNSIGNED_SHORT ? 65535f : 32767f) : x;
        }
        if (i != null) {
            if (p >= i.length || p < 0) {
                return 0f;
            }
            return type == FIXED ? i[p] / 65536f : (float) i[p];
        }
        if (p >= b.length || p < 0) {
            return 0f;
        }
        int x = type == UNSIGNED_BYTE ? b[p] & 255 : b[p];
        return normalized ? x / (type == UNSIGNED_BYTE ? 255f : 127f) : x;
    }
}
