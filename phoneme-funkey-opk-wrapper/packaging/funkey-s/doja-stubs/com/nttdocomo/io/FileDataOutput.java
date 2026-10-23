package com.nttdocomo.io;

import java.io.IOException;

public class FileDataOutput implements java.io.DataOutput, RandomAccessible {
    private FileEntity f;
    private int pos;
    private boolean closed;

    protected FileDataOutput() {
    }

    FileDataOutput(FileEntity f) {
        this.f = f;
    }

    private void check() throws IOException {
        if (closed || f.closed) {
            throw new IOException("closed");
        }
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

    public void truncate(long size) throws IOException {
        check();
        if (size < 0 || size > f.size) {
            throw new IllegalArgumentException("size");
        }
        f.size = (int) size;
        if (pos > f.size) {
            pos = f.size;
        }
        f.dirty = true;
    }

    public void flush() throws IOException {
        check();
        f.flushData();
    }

    public void close() throws IOException {
        if (!closed) {
            f.flushData();
            closed = true;
        }
    }

    public void writeString(String s) throws IOException {
        byte[] b = s.getBytes();
        if (b.length >= 65536) {
            throw new IllegalArgumentException("s");
        }
        writeShort(b.length);
        write(b);
    }

    public void writeString(String s, int bytes) throws IOException {
        byte[] b = s.getBytes();
        if (bytes < 0 || bytes >= 65536 || b.length > bytes) {
            throw new IllegalArgumentException("bytes");
        }
        writeShort(b.length);
        write(b);
        for (int i = b.length; i < bytes; i++) {
            write(0);
        }
    }

    public void write(byte[] b, int off, int len) throws IOException {
        check();
        f.ensure(pos + len);
        System.arraycopy(b, off, f.data, pos, len);
        pos += len;
        if (pos > f.size) {
            f.size = pos;
        }
        f.dirty = true;
    }

    public void write(byte[] b) throws IOException {
        write(b, 0, b.length);
    }

    public void write(int b) throws IOException {
        check();
        f.ensure(pos + 1);
        f.data[pos++] = (byte) b;
        if (pos > f.size) {
            f.size = pos;
        }
        f.dirty = true;
    }

    public void writeBoolean(boolean v) throws IOException {
        write(v ? 1 : 0);
    }

    public void writeByte(int v) throws IOException {
        write(v);
    }

    public void writeBytes(String s) throws IOException {
        for (int i = 0; i < s.length(); i++) {
            write(s.charAt(i));
        }
    }

    public void writeChar(int v) throws IOException {
        writeShort(v);
    }

    public void writeChars(String s) throws IOException {
        for (int i = 0; i < s.length(); i++) {
            writeChar(s.charAt(i));
        }
    }

    public void writeDouble(double v) throws IOException {
        writeLong(Double.doubleToLongBits(v));
    }

    public void writeFloat(float v) throws IOException {
        writeInt(Float.floatToIntBits(v));
    }

    public void writeInt(int v) throws IOException {
        write(v >>> 24);
        write(v >>> 16);
        write(v >>> 8);
        write(v);
    }

    public void writeLong(long v) throws IOException {
        writeInt((int) (v >>> 32));
        writeInt((int) v);
    }

    public void writeShort(int v) throws IOException {
        write(v >>> 8);
        write(v);
    }

    public void writeUTF(String s) throws IOException {
        java.io.ByteArrayOutputStream bo = new java.io.ByteArrayOutputStream();
        new java.io.DataOutputStream(bo).writeUTF(s);
        write(bo.toByteArray());
    }
}
