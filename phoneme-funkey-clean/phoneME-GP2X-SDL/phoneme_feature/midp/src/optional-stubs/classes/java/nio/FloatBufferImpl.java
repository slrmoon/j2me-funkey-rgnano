package java.nio;

final class FloatBufferImpl extends FloatBuffer {
    FloatBufferImpl(float[] array, int arrayOffset, byte[] bytes, int byteOffset, int cap, boolean direct) {
        super(array, arrayOffset, bytes, byteOffset, cap, direct);
    }

    private float read(int i) {
        if (array != null) {
            return array[arrayOffset + i];
        }
        int p = byteOffset + i * 4;
        return Float.intBitsToFloat(ByteBuffer.rdInt(bytes, p));
    }

    private void write(int i, float v) {
        if (array != null) {
            array[arrayOffset + i] = v;
            return;
        }
        int p = byteOffset + i * 4;
        ByteBuffer.wrInt(bytes, p, Float.floatToIntBits(v));
    }

    public FloatBuffer slice() {
        if (array != null) {
            return new FloatBufferImpl(array, arrayOffset + position, null, 0, remaining(), direct);
        }
        return new FloatBufferImpl(null, 0, bytes, byteOffset + position * 4, remaining(), direct);
    }

    public float get() {
        return read(next(1, false));
    }

    public FloatBuffer put(float v) {
        write(next(1, true), v);
        return this;
    }

    public float get(int index) {
        return read(check(index, 1));
    }

    public FloatBuffer put(int index, float v) {
        write(check(index, 1), v);
        return this;
    }

    public boolean isDirect() {
        return direct;
    }
}
