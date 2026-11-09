package com.nttdocomo.ui.ogl;

import com.nttdocomo.ui.ogl.math.Matrix4f;

/**
 * Buffers for GraphicsOGL vertex, index and texture data, kept as Java
 * arrays. A segment selects the part the GL calls read.
 */
public final class DirectBufferFactory {
    private static DirectBufferFactory factory;

    protected DirectBufferFactory() {
    }

    public static DirectBufferFactory getFactory() {
        if (factory == null) {
            factory = new DirectBufferFactory();
        }
        return factory;
    }

    private static void checkSize(int size) {
        if (size < 0) {
            throw new IllegalArgumentException("size");
        }
    }

    public ByteBuffer allocateByteBuffer(int size) {
        checkSize(size);
        return new ArrayByteBuffer(new byte[size]);
    }

    public ByteBuffer allocateByteBuffer(byte[] initialData) {
        return new ArrayByteBuffer(copy(initialData));
    }

    public ByteBuffer allocateByteBuffer(ByteBuffer buff) {
        ArrayByteBuffer b = (ArrayByteBuffer) buff;
        return new ArrayByteBuffer(copy(b.data));
    }

    public ByteBuffer allocateByteBuffer(short[] initialData) {
        if (initialData == null) {
            throw new NullPointerException("initialData");
        }
        byte[] d = new byte[initialData.length * 2];
        for (int i = 0; i < initialData.length; i++) {
            d[i * 2] = (byte) initialData[i];
            d[i * 2 + 1] = (byte) (initialData[i] >> 8);
        }
        return new ArrayByteBuffer(d);
    }

    public ShortBuffer allocateShortBuffer(int size) {
        checkSize(size);
        return new ArrayShortBuffer(new short[size]);
    }

    public ShortBuffer allocateShortBuffer(byte[] initialData) {
        if (initialData == null) {
            throw new NullPointerException("initialData");
        }
        short[] d = new short[initialData.length / 2];
        for (int i = 0; i < d.length; i++) {
            d[i] = (short) ((initialData[i * 2] & 255) | (initialData[i * 2 + 1] << 8));
        }
        return new ArrayShortBuffer(d);
    }

    public ShortBuffer allocateShortBuffer(ShortBuffer buff) {
        ArrayShortBuffer b = (ArrayShortBuffer) buff;
        short[] d = new short[b.data.length];
        System.arraycopy(b.data, 0, d, 0, d.length);
        return new ArrayShortBuffer(d);
    }

    public ShortBuffer allocateShortBuffer(short[] initialData) {
        if (initialData == null) {
            throw new NullPointerException("initialData");
        }
        short[] d = new short[initialData.length];
        System.arraycopy(initialData, 0, d, 0, d.length);
        return new ArrayShortBuffer(d);
    }

    public IntBuffer allocateIntBuffer(int size) {
        checkSize(size);
        return new ArrayIntBuffer(new int[size]);
    }

    public IntBuffer allocateIntBuffer(byte[] initialData) {
        if (initialData == null) {
            throw new NullPointerException("initialData");
        }
        int[] d = new int[initialData.length / 4];
        for (int i = 0; i < d.length; i++) {
            int p = i * 4;
            d[i] = (initialData[p] & 255) | ((initialData[p + 1] & 255) << 8)
                    | ((initialData[p + 2] & 255) << 16) | (initialData[p + 3] << 24);
        }
        return new ArrayIntBuffer(d);
    }

    public IntBuffer allocateIntBuffer(IntBuffer buff) {
        ArrayIntBuffer b = (ArrayIntBuffer) buff;
        int[] d = new int[b.data.length];
        System.arraycopy(b.data, 0, d, 0, d.length);
        return new ArrayIntBuffer(d);
    }

    public IntBuffer allocateIntBuffer(int[] initialData) {
        if (initialData == null) {
            throw new NullPointerException("initialData");
        }
        int[] d = new int[initialData.length];
        System.arraycopy(initialData, 0, d, 0, d.length);
        return new ArrayIntBuffer(d);
    }

    public FloatBuffer allocateFloatBuffer(int size) {
        checkSize(size);
        return new ArrayFloatBuffer(new float[size]);
    }

    public FloatBuffer allocateFloatBuffer(byte[] initialData) {
        if (initialData == null) {
            throw new NullPointerException("initialData");
        }
        float[] d = new float[initialData.length / 4];
        for (int i = 0; i < d.length; i++) {
            int p = i * 4;
            d[i] = Float.intBitsToFloat((initialData[p] & 255) | ((initialData[p + 1] & 255) << 8)
                    | ((initialData[p + 2] & 255) << 16) | (initialData[p + 3] << 24));
        }
        return new ArrayFloatBuffer(d);
    }

    public FloatBuffer allocateFloatBuffer(FloatBuffer buff) {
        ArrayFloatBuffer b = (ArrayFloatBuffer) buff;
        float[] d = new float[b.data.length];
        System.arraycopy(b.data, 0, d, 0, d.length);
        return new ArrayFloatBuffer(d);
    }

    public FloatBuffer allocateFloatBuffer(float[] initialData) {
        if (initialData == null) {
            throw new NullPointerException("initialData");
        }
        float[] d = new float[initialData.length];
        System.arraycopy(initialData, 0, d, 0, d.length);
        return new ArrayFloatBuffer(d);
    }

    private static byte[] copy(byte[] b) {
        if (b == null) {
            throw new NullPointerException("initialData");
        }
        byte[] d = new byte[b.length];
        System.arraycopy(b, 0, d, 0, d.length);
        return d;
    }

    public static int getSegmentOffset(DirectBuffer buff) {
        return ((AbstractBuffer) buff).segOff;
    }

    public static int getSegmentLength(DirectBuffer buff) {
        AbstractBuffer b = (AbstractBuffer) buff;
        return b.segLen < 0 ? buff.length() - b.segOff : b.segLen;
    }

    public static short getShort(ShortBuffer buff, int index) {
        return ((ArrayShortBuffer) buff).data[index];
    }

    public static float getFloat(FloatBuffer buff, int index) {
        return ((ArrayFloatBuffer) buff).data[index];
    }

    /** the backing array of a buffer (runtime use) */
    public static Object array(DirectBuffer buff) {
        if (buff instanceof ArrayFloatBuffer) {
            return ((ArrayFloatBuffer) buff).data;
        }
        if (buff instanceof ArrayShortBuffer) {
            return ((ArrayShortBuffer) buff).data;
        }
        if (buff instanceof ArrayIntBuffer) {
            return ((ArrayIntBuffer) buff).data;
        }
        if (buff instanceof ArrayByteBuffer) {
            return ((ArrayByteBuffer) buff).data;
        }
        return null;
    }

    public abstract static class AbstractBuffer {
        int segOff;
        int segLen = -1;

        protected AbstractBuffer() {
        }

        abstract int size();

        public int length() {
            return size();
        }

        public void setSegment(int offset, int length) {
            if (offset < 0 || length < 0 || offset + length > size()) {
                throw new ArrayIndexOutOfBoundsException();
            }
            segOff = offset;
            segLen = length;
        }

        public void clearSegment() {
            segOff = 0;
            segLen = -1;
        }

        static void check(int index, int len, int arrLen, int size) {
            if (index < 0 || len < 0 || index + len > size || len > arrLen) {
                throw new ArrayIndexOutOfBoundsException();
            }
        }
    }

    public static final class ArrayByteBuffer extends AbstractBuffer implements ByteBuffer {
        byte[] data;

        public ArrayByteBuffer(byte[] values) {
            data = values;
        }

        protected ArrayByteBuffer() {
            data = new byte[0];
        }

        int size() {
            return data.length;
        }

        public byte[] get(int index, byte[] buff) {
            return get(index, buff, 0, buff.length);
        }

        public byte[] get(int index, byte[] buff, int offset, int length) {
            check(index, length, buff.length - offset, data.length);
            System.arraycopy(data, index, buff, offset, length);
            return buff;
        }

        public void put(int index, byte[] buff) {
            put(index, buff, 0, buff.length);
        }

        public void put(int index, byte[] buff, int offset, int length) {
            check(index, length, buff.length - offset, data.length);
            System.arraycopy(buff, offset, data, index, length);
        }
    }

    public static final class ArrayShortBuffer extends AbstractBuffer implements ShortBuffer {
        short[] data;

        public ArrayShortBuffer(short[] values) {
            data = values;
        }

        protected ArrayShortBuffer() {
            data = new short[0];
        }

        int size() {
            return data.length;
        }

        public short[] get(int index, short[] buff) {
            return get(index, buff, 0, buff.length);
        }

        public short[] get(int index, short[] buff, int offset, int length) {
            check(index, length, buff.length - offset, data.length);
            System.arraycopy(data, index, buff, offset, length);
            return buff;
        }

        public void put(int index, short[] buff) {
            put(index, buff, 0, buff.length);
        }

        public void put(int index, short[] buff, int offset, int length) {
            check(index, length, buff.length - offset, data.length);
            System.arraycopy(buff, offset, data, index, length);
        }
    }

    public static final class ArrayIntBuffer extends AbstractBuffer implements IntBuffer {
        int[] data;

        public ArrayIntBuffer(int[] values) {
            data = values;
        }

        protected ArrayIntBuffer() {
            data = new int[0];
        }

        int size() {
            return data.length;
        }

        public int[] get(int index, int[] buff) {
            return get(index, buff, 0, buff.length);
        }

        public int[] get(int index, int[] buff, int offset, int length) {
            check(index, length, buff.length - offset, data.length);
            System.arraycopy(data, index, buff, offset, length);
            return buff;
        }

        public void put(int index, int[] buff) {
            put(index, buff, 0, buff.length);
        }

        public void put(int index, int[] buff, int offset, int length) {
            check(index, length, buff.length - offset, data.length);
            System.arraycopy(buff, offset, data, index, length);
        }
    }

    public static final class ArrayFloatBuffer extends AbstractBuffer implements FloatBuffer {
        float[] data;

        public ArrayFloatBuffer(float[] values) {
            data = values;
        }

        protected ArrayFloatBuffer() {
            data = new float[0];
        }

        int size() {
            return data.length;
        }

        public float[] get(int index, float[] buff) {
            return get(index, buff, 0, buff.length);
        }

        public float[] get(int index, float[] buff, int offset, int length) {
            check(index, length, buff.length - offset, data.length);
            System.arraycopy(data, index, buff, offset, length);
            return buff;
        }

        public void put(int index, float[] buff) {
            put(index, buff, 0, buff.length);
        }

        public void put(int index, float[] buff, int offset, int length) {
            check(index, length, buff.length - offset, data.length);
            System.arraycopy(buff, offset, data, index, length);
        }

        /** this = src1 + src2 * multiplier (element-wise over the segments) */
        public FloatBuffer madd(FloatBuffer src1, FloatBuffer src2, float multiplier) {
            float[] a = ((ArrayFloatBuffer) src1).data;
            float[] b = ((ArrayFloatBuffer) src2).data;
            int oa = getSegmentOffset(src1);
            int ob = getSegmentOffset(src2);
            int od = segOff;
            int n = getSegmentLength(this);
            n = java.lang.Math.min(n, java.lang.Math.min(getSegmentLength(src1), getSegmentLength(src2)));
            for (int i = 0; i < n; i++) {
                data[od + i] = a[oa + i] + b[ob + i] * multiplier;
            }
            return this;
        }

        /** this = matrix * items of src (itemSize 3 or 4) */
        public FloatBuffer transform(FloatBuffer src, Matrix4f matrix, int itemSize, int itemCount) {
            float[] s = ((ArrayFloatBuffer) src).data;
            float[] m = matrix.m;
            int os = getSegmentOffset(src);
            int od = segOff;
            for (int i = 0; i < itemCount; i++) {
                int p = os + i * itemSize;
                float x = s[p];
                float y = s[p + 1];
                float z = itemSize > 2 ? s[p + 2] : 0f;
                float w = itemSize > 3 ? s[p + 3] : 1f;
                int q = od + i * itemSize;
                data[q] = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
                data[q + 1] = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
                if (itemSize > 2) {
                    data[q + 2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
                }
                if (itemSize > 3) {
                    data[q + 3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;
                }
            }
            return this;
        }
    }
}
