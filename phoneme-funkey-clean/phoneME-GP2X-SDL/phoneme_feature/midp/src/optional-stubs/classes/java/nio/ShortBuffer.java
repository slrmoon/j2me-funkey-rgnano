package java.nio;

/**
 * JSR 239 subset of java.nio.ShortBuffer: wraps a short[] or views a ByteBuffer
 * (native little-endian order). FunKey: Java heap only.
 */
public abstract class ShortBuffer extends Buffer implements Comparable {
    /* array-backed: array and offset; byte view: bytes and byte offset */
    final short[] array;
    final int arrayOffset;
    final byte[] bytes;
    final int byteOffset;
    final boolean direct;

    ShortBuffer(short[] array, int arrayOffset, byte[] bytes, int byteOffset, int cap, boolean direct) {
        this.array = array;
        this.arrayOffset = arrayOffset;
        this.bytes = bytes;
        this.byteOffset = byteOffset;
        this.direct = direct;
        init(cap);
    }

    public static ShortBuffer wrap(short[] array, int offset, int length) {
        checkBounds(offset, length, array.length);
        ShortBuffer b = new ShortBufferImpl(array, 0, null, 0, array.length, false);
        b.position = offset;
        b.limit = offset + length;
        return b;
    }

    public static ShortBuffer wrap(short[] array) {
        return wrap(array, 0, array.length);
    }

    public abstract ShortBuffer slice();

    public abstract short get();

    public abstract ShortBuffer put(short v);

    public abstract short get(int index);

    public abstract ShortBuffer put(int index, short v);

    public ShortBuffer get(short[] dst, int offset, int length) {
        checkBounds(offset, length, dst.length);
        if (remaining() < length) {
            throw new BufferUnderflowException();
        }
        for (int i = 0; i < length; i++) {
            dst[offset + i] = get();
        }
        return this;
    }

    public ShortBuffer get(short[] dst) {
        return get(dst, 0, dst.length);
    }

    public ShortBuffer put(ShortBuffer src) {
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

    public ShortBuffer put(short[] src, int offset, int length) {
        checkBounds(offset, length, src.length);
        if (remaining() < length) {
            throw new BufferOverflowException();
        }
        for (int i = 0; i < length; i++) {
            put(src[offset + i]);
        }
        return this;
    }

    public final ShortBuffer put(short[] src) {
        return put(src, 0, src.length);
    }

    public final boolean hasArray() {
        return array != null && !direct;
    }

    public final short[] array() {
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
        return "java.nio.ShortBuffer[pos=" + position + " lim=" + limit + " cap=" + capacity + "]";
    }

    public int hashCode() {
        int h = 1;
        for (int i = limit - 1; i >= position; i--) {
            h = 31 * h + (int) get(i);
        }
        return h;
    }

    public boolean equals(Object ob) {
        if (!(ob instanceof ShortBuffer)) {
            return false;
        }
        ShortBuffer o = (ShortBuffer) ob;
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
        ShortBuffer o = (ShortBuffer) ob;
        int n = Math.min(remaining(), o.remaining());
        for (int i = 0; i < n; i++) {
            short x = get(position + i);
            short y = o.get(o.position + i);
            if (x != y) {
                return x < y ? -1 : 1;
            }
        }
        return remaining() - o.remaining();
    }
}
