package java.nio;

final class ByteBufferImpl extends ByteBuffer {
    ByteBufferImpl(byte[] hb, int offset, int cap, boolean direct) {
        super(hb, offset, cap, direct);
    }

    public ByteBuffer slice() {
        return new ByteBufferImpl(hb, offset + position, remaining(), direct);
    }

    public byte get() {
        return hb[offset + next(1, false)];
    }

    public ByteBuffer put(byte b) {
        hb[offset + next(1, true)] = b;
        return this;
    }

    public byte get(int index) {
        return hb[offset + check(index, 1)];
    }

    public ByteBuffer put(int index, byte b) {
        hb[offset + check(index, 1)] = b;
        return this;
    }

    public boolean isDirect() {
        return direct;
    }

    public short getShort() {
        return rdShort(hb, offset + next(2, false));
    }

    public ByteBuffer putShort(short value) {
        wrShort(hb, offset + next(2, true), value);
        return this;
    }

    public short getShort(int index) {
        return rdShort(hb, offset + check(index, 2));
    }

    public ByteBuffer putShort(int index, short value) {
        wrShort(hb, offset + check(index, 2), value);
        return this;
    }

    public ShortBuffer asShortBuffer() {
        return new ShortBufferImpl(null, 0, hb, offset + position, remaining() >> 1, direct);
    }

    public int getInt() {
        return rdInt(hb, offset + next(4, false));
    }

    public ByteBuffer putInt(int value) {
        wrInt(hb, offset + next(4, true), value);
        return this;
    }

    public int getInt(int index) {
        return rdInt(hb, offset + check(index, 4));
    }

    public ByteBuffer putInt(int index, int value) {
        wrInt(hb, offset + check(index, 4), value);
        return this;
    }

    public IntBuffer asIntBuffer() {
        return new IntBufferImpl(null, 0, hb, offset + position, remaining() >> 2, direct);
    }

    public float getFloat() {
        return Float.intBitsToFloat(getInt());
    }

    public ByteBuffer putFloat(float value) {
        return putInt(Float.floatToIntBits(value));
    }

    public float getFloat(int index) {
        return Float.intBitsToFloat(getInt(index));
    }

    public ByteBuffer putFloat(int index, float value) {
        return putInt(index, Float.floatToIntBits(value));
    }

    public FloatBuffer asFloatBuffer() {
        return new FloatBufferImpl(null, 0, hb, offset + position, remaining() >> 2, direct);
    }
}
