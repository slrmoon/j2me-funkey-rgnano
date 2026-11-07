package java.nio;

/**
 * JSR 239 subset of java.nio.ByteBuffer: direct or array-backed, values
 * in the native (little-endian) byte order. FunKey: everything lives on
 * the Java heap; a direct buffer just has no accessible array.
 */
public abstract class ByteBuffer extends Buffer implements Comparable {
    final byte[] hb;
    final int offset;
    final boolean direct;

    ByteBuffer(byte[] hb, int offset, int cap, boolean direct) {
        this.hb = hb;
        this.offset = offset;
        this.direct = direct;
        init(cap);
    }

    public static ByteBuffer allocateDirect(int capacity) {
        if (capacity < 0) {
            throw new IllegalArgumentException();
        }
        return new ByteBufferImpl(new byte[capacity], 0, capacity, true);
    }

    public static ByteBuffer wrap(byte[] array, int offset, int length) {
        checkBounds(offset, length, array.length);
        ByteBuffer b = new ByteBufferImpl(array, 0, array.length, false);
        b.position = offset;
        b.limit = offset + length;
        return b;
    }

    public static ByteBuffer wrap(byte[] array) {
        return wrap(array, 0, array.length);
    }

    public abstract ByteBuffer slice();

    public abstract byte get();

    public abstract ByteBuffer put(byte b);

    public abstract byte get(int index);

    public abstract ByteBuffer put(int index, byte b);

    public ByteBuffer get(byte[] dst, int offset, int length) {
        checkBounds(offset, length, dst.length);
        int p = next(length, false);
        System.arraycopy(hb, this.offset + p, dst, offset, length);
        return this;
    }

    public ByteBuffer get(byte[] dst) {
        return get(dst, 0, dst.length);
    }

    public ByteBuffer put(ByteBuffer src) {
        if (src == this) {
            throw new IllegalArgumentException();
        }
        int n = src.remaining();
        int p = next(n, true);
        System.arraycopy(src.hb, src.offset + src.position, hb, offset + p, n);
        src.position += n;
        return this;
    }

    public ByteBuffer put(byte[] src, int offset, int length) {
        checkBounds(offset, length, src.length);
        int p = next(length, true);
        System.arraycopy(src, offset, hb, this.offset + p, length);
        return this;
    }

    public final ByteBuffer put(byte[] src) {
        return put(src, 0, src.length);
    }

    public final boolean hasArray() {
        return !direct;
    }

    public final byte[] array() {
        if (direct) {
            throw new UnsupportedOperationException();
        }
        return hb;
    }

    public final int arrayOffset() {
        if (direct) {
            throw new UnsupportedOperationException();
        }
        return offset;
    }

    public abstract boolean isDirect();

    public String toString() {
        return "java.nio.ByteBuffer[pos=" + position + " lim=" + limit + " cap=" + capacity + "]";
    }

    public int hashCode() {
        int h = 1;
        for (int i = limit - 1; i >= position; i--) {
            h = 31 * h + get(i);
        }
        return h;
    }

    public boolean equals(Object ob) {
        if (!(ob instanceof ByteBuffer)) {
            return false;
        }
        ByteBuffer o = (ByteBuffer) ob;
        if (remaining() != o.remaining()) {
            return false;
        }
        for (int i = 0; i < remaining(); i++) {
            if (get(position + i) != o.get(o.position + i)) {
                return false;
            }
        }
        return true;
    }

    public int compareTo(Object ob) {
        ByteBuffer o = (ByteBuffer) ob;
        int n = Math.min(remaining(), o.remaining());
        for (int i = 0; i < n; i++) {
            int a = get(position + i);
            int b = o.get(o.position + i);
            if (a != b) {
                return a < b ? -1 : 1;
            }
        }
        return remaining() - o.remaining();
    }

    /* little-endian element access at a byte index of hb */
    static short rdShort(byte[] b, int p) {
        return (short) ((b[p] & 255) | (b[p + 1] << 8));
    }

    static void wrShort(byte[] b, int p, short v) {
        b[p] = (byte) v;
        b[p + 1] = (byte) (v >> 8);
    }

    static int rdInt(byte[] b, int p) {
        return (b[p] & 255) | ((b[p + 1] & 255) << 8) | ((b[p + 2] & 255) << 16) | (b[p + 3] << 24);
    }

    static void wrInt(byte[] b, int p, int v) {
        b[p] = (byte) v;
        b[p + 1] = (byte) (v >> 8);
        b[p + 2] = (byte) (v >> 16);
        b[p + 3] = (byte) (v >> 24);
    }

    public abstract short getShort();

    public abstract ByteBuffer putShort(short value);

    public abstract short getShort(int index);

    public abstract ByteBuffer putShort(int index, short value);

    public abstract ShortBuffer asShortBuffer();

    public abstract int getInt();

    public abstract ByteBuffer putInt(int value);

    public abstract int getInt(int index);

    public abstract ByteBuffer putInt(int index, int value);

    public abstract IntBuffer asIntBuffer();

    public abstract float getFloat();

    public abstract ByteBuffer putFloat(float value);

    public abstract float getFloat(int index);

    public abstract ByteBuffer putFloat(int index, float value);

    public abstract FloatBuffer asFloatBuffer();
}
