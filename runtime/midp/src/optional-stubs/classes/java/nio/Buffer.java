package java.nio;

/**
 * JSR 239 subset of java.nio.Buffer (FunKey: Java heap buffers; the
 * OpenGL ES that would read them natively is not available).
 */
public abstract class Buffer {
    int capacity;
    int limit;
    int position;

    Buffer() {
    }

    public final int capacity() {
        return capacity;
    }

    public final int position() {
        return position;
    }

    public final Buffer position(int newPosition) {
        if (newPosition < 0 || newPosition > limit) {
            throw new IllegalArgumentException();
        }
        position = newPosition;
        return this;
    }

    public final int limit() {
        return limit;
    }

    public final Buffer limit(int newLimit) {
        if (newLimit < 0 || newLimit > capacity) {
            throw new IllegalArgumentException();
        }
        limit = newLimit;
        if (position > newLimit) {
            position = newLimit;
        }
        return this;
    }

    public final Buffer clear() {
        position = 0;
        limit = capacity;
        return this;
    }

    public final Buffer flip() {
        limit = position;
        position = 0;
        return this;
    }

    public final Buffer rewind() {
        position = 0;
        return this;
    }

    public final int remaining() {
        return limit - position;
    }

    public final boolean hasRemaining() {
        return position < limit;
    }

    /** index of the next relative get/put of n elements */
    final int next(int n, boolean put) {
        if (limit - position < n) {
            if (put) {
                throw new BufferOverflowException();
            }
            throw new BufferUnderflowException();
        }
        int p = position;
        position += n;
        return p;
    }

    /** checks an absolute index for an element of n units */
    final int check(int index, int n) {
        if (index < 0 || index > limit - n) {
            throw new IndexOutOfBoundsException();
        }
        return index;
    }

    static void checkBounds(int offset, int length, int size) {
        if ((offset | length) < 0 || offset > size - length) {
            throw new IndexOutOfBoundsException();
        }
    }

    final void init(int cap) {
        capacity = cap;
        limit = cap;
        position = 0;
    }
}
