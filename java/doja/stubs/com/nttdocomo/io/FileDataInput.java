package com.nttdocomo.io;

import java.io.DataInputStream;
import java.io.EOFException;
import java.io.IOException;

public class FileDataInput implements java.io.DataInput, RandomAccessible {
    private FileEntity f;
    private int pos;
    private boolean closed;

    protected FileDataInput() {
    }

    FileDataInput(FileEntity f) {
        this.f = f;
    }

    private void check() throws IOException {
        if (closed || f.closed) {
            throw new IOException("closed");
        }
    }

    private int next() throws IOException {
        check();
        if (pos >= f.size) {
            throw new EOFException();
        }
        return f.data[pos++] & 0xFF;
    }

    public long getSize() throws IOException {
        check();
        return f.size;
    }

    public long getPosition() throws IOException {
        check();
        return pos;
    }

    public void setPosition(long position) throws IOException {
        check();
        if (position < 0 || position > f.size) {
            throw new IllegalArgumentException("position");
        }
        pos = (int) position;
    }

    public void setPositionRelative(long position) throws IOException {
        setPosition(pos + position);
    }

    public void close() throws IOException {
        closed = true;
    }

    public String readString() throws IOException {
        int n = readUnsignedShort();
        byte[] b = new byte[n];
        readFully(b);
        return new String(b);
    }

    public String readString(int bytes) throws IOException {
        if (bytes < 0 || bytes >= 65536) {
            throw new IllegalArgumentException("bytes");
        }
        int n = readUnsignedShort();
        if (n > bytes) {
            throw new IllegalArgumentException("bytes");
        }
        byte[] b = new byte[n];
        readFully(b);
        if (bytes > n) {
            setPositionRelative(bytes - n);
        }
        return new String(b);
    }

    public boolean readBoolean() throws IOException {
        return next() != 0;
    }

    public byte readByte() throws IOException {
        return (byte) next();
    }

    public char readChar() throws IOException {
        return (char) readUnsignedShort();
    }

    public double readDouble() throws IOException {
        return Double.longBitsToDouble(readLong());
    }

    public float readFloat() throws IOException {
        return Float.intBitsToFloat(readInt());
    }

    public void readFully(byte[] b, int off, int len) throws IOException {
        check();
        if (len > f.size - pos) {
            throw new EOFException();
        }
        System.arraycopy(f.data, pos, b, off, len);
        pos += len;
    }

    public void readFully(byte[] b) throws IOException {
        readFully(b, 0, b.length);
    }

    public int readInt() throws IOException {
        return next() << 24 | next() << 16 | next() << 8 | next();
    }

    public String readLine() throws IOException {
        check();
        if (pos >= f.size) {
            return null;
        }
        StringBuffer sb = new StringBuffer();
        while (pos < f.size) {
            int c = f.data[pos++] & 0xFF;
            if (c == '\n') {
                break;
            }
            if (c == '\r') {
                if (pos < f.size && f.data[pos] == '\n') {
                    pos++;
                }
                break;
            }
            sb.append((char) c);
        }
        return sb.toString();
    }

    public long readLong() throws IOException {
        return ((long) readInt() << 32) | (readInt() & 0xFFFFFFFFL);
    }

    public short readShort() throws IOException {
        return (short) readUnsignedShort();
    }

    public int readUnsignedByte() throws IOException {
        return next();
    }

    public int readUnsignedShort() throws IOException {
        return next() << 8 | next();
    }

    public String readUTF() throws IOException {
        return DataInputStream.readUTF(this);
    }

    public int skipBytes(int n) throws IOException {
        check();
        int k = Math.max(0, Math.min(n, f.size - pos));
        pos += k;
        return k;
    }
}
