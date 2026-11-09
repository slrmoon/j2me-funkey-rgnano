package java.nio;

final class IntBufferImpl extends IntBuffer {
    IntBufferImpl(int[] array, int arrayOffset, byte[] bytes, int byteOffset, int cap, boolean direct) {
        super(array, arrayOffset, bytes, byteOffset, cap, direct);
    }

    private int read(int i) {
        if (array != null) {
            return array[arrayOffset + i];
        }
        int p = byteOffset + i * 4;
        return ByteBuffer.rdInt(bytes, p);
    }

    private void write(int i, int v) {
        if (array != null) {
            array[arrayOffset + i] = v;
            return;
        }
        int p = byteOffset + i * 4;
        ByteBuffer.wrInt(bytes, p, v);
    }

    public IntBuffer slice() {
        if (array != null) {
            return new IntBufferImpl(array, arrayOffset + position, null, 0, remaining(), direct);
        }
        return new IntBufferImpl(null, 0, bytes, byteOffset + position * 4, remaining(), direct);
    }

    public int get() {
        return read(next(1, false));
    }

    public IntBuffer put(int v) {
        write(next(1, true), v);
        return this;
    }

    public int get(int index) {
        return read(check(index, 1));
    }

    public IntBuffer put(int index, int v) {
        write(check(index, 1), v);
        return this;
    }

    public boolean isDirect() {
        return direct;
    }
}
