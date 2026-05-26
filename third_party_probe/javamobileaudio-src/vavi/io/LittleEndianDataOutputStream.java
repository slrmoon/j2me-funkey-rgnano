/*
 * Copyright (c) 2004 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.io;

import java.io.FilterOutputStream;
import java.io.IOException;
import java.io.OutputStream;


/**
 * LittleEndianDataOutputStream.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 040826 nsano initial version <br>
 */
public class LittleEndianDataOutputStream extends FilterOutputStream
    implements LittleEndianDataOutput {

    /**
     * Creates Little Endian OutputStream.
     */
    public LittleEndianDataOutputStream(OutputStream out) {
        super(out);
    }

    /** @see java.io.DataOutput#writeDouble(double) */
    public void writeDouble(double d) throws IOException {
        writeLong(Double.doubleToLongBits(d));
    }

    /** @see java.io.DataOutput#writeFloat(float) */
    public void writeFloat(float f) throws IOException {
        writeInt(Float.floatToIntBits(f));
    }

    /** @see java.io.DataOutput#writeByte(int) */
    public void writeByte(int b) throws IOException {
        out.write(b);
    }

    /** @see java.io.DataOutput#writeChar(int) */
    public void writeChar(int c) throws IOException {
        throw new UnsupportedOperationException("not implemented");
    }

    /** @see java.io.DataOutput#writeInt(int) */
    public void writeInt(int i) throws IOException {
        writeShort((i & 0xffff));
        writeShort(((i >> 16) & 0xffff));
    }

    /** @see java.io.DataOutput#writeShort(int) */
    public void writeShort(int i) throws IOException {
        out.write(i & 0xff);
        out.write((i >> 8) & 0xff);
    }

    /** @see java.io.DataOutput#writeLong(long) */
    public void writeLong(long l) throws IOException {
        writeInt((int) (l & 0xffffffff));
        writeInt((int) ((l >> 32) & 0xffffffff));
    }

    /** @see java.io.DataOutput#writeBoolean(boolean) */
    public void writeBoolean(boolean b) throws IOException {
        throw new UnsupportedOperationException("not implemented");
    }

    /** @see java.io.DataOutput#writeBytes(java.lang.String) */
    public void writeBytes(String s) throws IOException {
        for (byte b : s.getBytes()) {
            writeByte(b);
        }
    }

    /** @see java.io.DataOutput#writeChars(java.lang.String) */
    public void writeChars(String s) throws IOException {
        throw new UnsupportedOperationException("not implemented");
    }

    /** @see java.io.DataOutput#writeUTF(java.lang.String) */
    public void writeUTF(String s) throws IOException {
        throw new UnsupportedOperationException("not implemented");
    }
}

/* */
