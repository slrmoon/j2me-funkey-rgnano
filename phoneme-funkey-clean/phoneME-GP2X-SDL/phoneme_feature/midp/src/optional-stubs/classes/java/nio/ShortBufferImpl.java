package java.nio;

final class ShortBufferImpl extends ShortBuffer {
    ShortBufferImpl(short[] array, int arrayOffset, byte[] bytes, int byteOffset, int cap, boolean direct) {
        super(array, arrayOffset, bytes, byteOffset, cap, direct);
    }

    private short read(int i) {
        if (array != null) {
            return array[arrayOffset + i];
        }
        int p = byteOffset + i * 2;
        return ByteBuffer.rdShort(bytes, p);
    }

    private void write(int i, short v) {
        if (array != null) {
            array[arrayOffset + i] = v;
            return;
        }
        int p = byteOffset + i * 2;
        ByteBuffer.wrShort(bytes, p, v);
    }

    public ShortBuffer slice() {
        if (array != null) {
            return new ShortBufferImpl(array, arrayOffset + position, null, 0, remaining(), direct);
        }
        return new ShortBufferImpl(null, 0, bytes, byteOffset + position * 2, remaining(), direct);
    }

    public short get() {
        return read(next(1, false));
    }

    public ShortBuffer put(short v) {
        write(next(1, true), v);
        return this;
    }

    public short get(int index) {
        return read(check(index, 1));
    }

    public ShortBuffer put(int index, short v) {
        write(check(index, 1), v);
        return this;
    }

    public boolean isDirect() {
        return direct;
    }
}
