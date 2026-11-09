package java.nio;

/**
 * JSR 239 subset of java.nio.FloatBuffer: wraps a float[] or views a ByteBuffer
 * (native little-endian order). FunKey: Java heap only.
 */
public abstract class FloatBuffer extends Buffer implements Comparable {
    /* array-backed: array and offset; byte view: bytes and byte offset */
    final float[] array;
    final int arrayOffset;
    final byte[] bytes;
    final int byteOffset;
    final boolean direct;

    FloatBuffer(float[] array, int arrayOffset, byte[] bytes, int byteOffset, int cap, boolean direct) {
        this.array = array;
        this.arrayOffset = arrayOffset;
        this.bytes = bytes;
        this.byteOffset = byteOffset;
        this.direct = direct;
        init(cap);
    }

    public static FloatBuffer wrap(float[] array, int offset, int length) {
        checkBounds(offset, length, array.length);
        FloatBuffer b = new FloatBufferImpl(array, 0, null, 0, array.length, false);
        b.position = offset;
        b.limit = offset + length;
        return b;
    }

    public static FloatBuffer wrap(float[] array) {
        return wrap(array, 0, array.length);
    }

    public abstract FloatBuffer slice();

    public abstract float get();

    public abstract FloatBuffer put(float v);

    public abstract float get(int index);

    public abstract FloatBuffer put(int index, float v);

    public FloatBuffer get(float[] dst, int offset, int length) {
        checkBounds(offset, length, dst.length);
        if (remaining() < length) {
            throw new BufferUnderflowException();
        }
        for (int i = 0; i < length; i++) {
            dst[offset + i] = get();
        }
        return this;
    }

    public FloatBuffer get(float[] dst) {
        return get(dst, 0, dst.length);
    }

    public FloatBuffer put(FloatBuffer src) {
        if (src == this) {
            throw new IllegalArgumentException();
        }
        if (remaining() < src.remaining()) {
            throw new BufferOverflowException();
        }
        while (src.hasRemaining()) {
            put(src.get());
        }
        return this;
    }

    public FloatBuffer put(float[] src, int offset, int length) {
        checkBounds(offset, length, src.length);
        if (remaining() < length) {
            throw new BufferOverflowException();
        }
        for (int i = 0; i < length; i++) {
            put(src[offset + i]);
        }
        return this;
    }

    public final FloatBuffer put(float[] src) {
        return put(src, 0, src.length);
    }

    public final boolean hasArray() {
        return array != null && !direct;
    }

    public final float[] array() {
        if (!hasArray()) {
            throw new UnsupportedOperationException();
        }
        return array;
    }

    public final int arrayOffset() {
        if (!hasArray()) {
            throw new UnsupportedOperationException();
        }
        return arrayOffset;
    }

    public abstract boolean isDirect();

    public String toString() {
        return "java.nio.FloatBuffer[pos=" + position + " lim=" + limit + " cap=" + capacity + "]";
    }

    public int hashCode() {
        int h = 1;
        for (int i = limit - 1; i >= position; i--) {
            h = 31 * h + Float.floatToIntBits(get(i));
        }
        return h;
    }

    public boolean equals(Object ob) {
        if (!(ob instanceof FloatBuffer)) {
            return false;
        }
        FloatBuffer o = (FloatBuffer) ob;
        if (remaining() != o.remaining()) {
            return false;
        }
        for (int i = 0; i < remaining(); i++) {
            if (Float.floatToIntBits(get(position + i)) != Float.floatToIntBits(o.get(o.position + i))) {
                return false;
            }
        }
        return true;
    }

    public int compareTo(Object ob) {
        FloatBuffer o = (FloatBuffer) ob;
        int n = Math.min(remaining(), o.remaining());
        for (int i = 0; i < n; i++) {
            float x = get(position + i);
            float y = o.get(o.position + i);
            if (x != y) {
                return x < y ? -1 : 1;
            }
        }
        return remaining() - o.remaining();
    }
}
