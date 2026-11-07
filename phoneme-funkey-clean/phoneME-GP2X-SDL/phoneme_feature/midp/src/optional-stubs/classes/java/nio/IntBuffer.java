package java.nio;

/**
 * JSR 239 subset of java.nio.IntBuffer: wraps a int[] or views a ByteBuffer
 * (native little-endian order). FunKey: Java heap only.
 */
public abstract class IntBuffer extends Buffer implements Comparable {
    /* array-backed: array and offset; byte view: bytes and byte offset */
    final int[] array;
    final int arrayOffset;
    final byte[] bytes;
    final int byteOffset;
    final boolean direct;

    IntBuffer(int[] array, int arrayOffset, byte[] bytes, int byteOffset, int cap, boolean direct) {
        this.array = array;
        this.arrayOffset = arrayOffset;
        this.bytes = bytes;
        this.byteOffset = byteOffset;
        this.direct = direct;
        init(cap);
    }

    public static IntBuffer wrap(int[] array, int offset, int length) {
        checkBounds(offset, length, array.length);
        IntBuffer b = new IntBufferImpl(array, 0, null, 0, array.length, false);
        b.position = offset;
        b.limit = offset + length;
        return b;
    }

    public static IntBuffer wrap(int[] array) {
        return wrap(array, 0, array.length);
    }

    public abstract IntBuffer slice();

    public abstract int get();

    public abstract IntBuffer put(int v);

    public abstract int get(int index);

    public abstract IntBuffer put(int index, int v);

    public IntBuffer get(int[] dst, int offset, int length) {
        checkBounds(offset, length, dst.length);
        if (remaining() < length) {
            throw new BufferUnderflowException();
        }
        for (int i = 0; i < length; i++) {
            dst[offset + i] = get();
        }
        return this;
    }

    public IntBuffer get(int[] dst) {
        return get(dst, 0, dst.length);
    }

    public IntBuffer put(IntBuffer src) {
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

    public IntBuffer put(int[] src, int offset, int length) {
        checkBounds(offset, length, src.length);
        if (remaining() < length) {
            throw new BufferOverflowException();
        }
        for (int i = 0; i < length; i++) {
            put(src[offset + i]);
        }
        return this;
    }

    public final IntBuffer put(int[] src) {
        return put(src, 0, src.length);
    }

    public final boolean hasArray() {
        return array != null && !direct;
    }

    public final int[] array() {
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
        return "java.nio.IntBuffer[pos=" + position + " lim=" + limit + " cap=" + capacity + "]";
    }

    public int hashCode() {
        int h = 1;
        for (int i = limit - 1; i >= position; i--) {
            h = 31 * h + (int) get(i);
        }
        return h;
    }

    public boolean equals(Object ob) {
        if (!(ob instanceof IntBuffer)) {
            return false;
        }
        IntBuffer o = (IntBuffer) ob;
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
        IntBuffer o = (IntBuffer) ob;
        int n = Math.min(remaining(), o.remaining());
        for (int i = 0; i < n; i++) {
            int x = get(position + i);
            int y = o.get(o.position + i);
            if (x != y) {
                return x < y ? -1 : 1;
            }
        }
        return remaining() - o.remaining();
    }
}
