/*
 * Copyright (c) 2004 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.io;

import java.io.DataInput;
import java.io.IOException;


/**
 * LittleEndianDataInput.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 040929 nsano initial version <br>
 */
public interface LittleEndianDataInput extends DataInput {
    /** @see java.io.DataInput#readShort() */
    short readShort() throws IOException;

    /** @see java.io.DataInput#readInt() */
    int readInt() throws IOException;

    /** @see java.io.DataInput#readLong() */
    long readLong() throws IOException;

    /** @see java.io.DataInput#readUTF() */
    String readUTF() throws IOException;

    /** @see java.io.DataInput#readLine() */
    String readLine() throws IOException;

    /** @see java.io.DataInput#readFully(byte[], int, int) */
    void readFully(byte[] b, int offset, int len) throws IOException;

    /** @see java.io.DataInput#readFully(byte[]) */
    void readFully(byte[] b) throws IOException;

    /** @see java.io.DataInput#skipBytes(int) */
    int skipBytes(int len) throws IOException;

    /** @see java.io.DataInput#readBoolean() */
    boolean readBoolean() throws IOException;

    /** @see java.io.DataInput#readUnsignedShort() */
    int readUnsignedShort() throws IOException;

    /** @see java.io.DataInput#readUnsignedByte() */
    int readUnsignedByte() throws IOException;

    /** @see java.io.DataInput#readFloat() */
    float readFloat() throws IOException;

    /** @see java.io.DataInput#readDouble() */
    double readDouble() throws IOException;

    /** @see java.io.DataInput#readChar() */
    char readChar() throws IOException;

    /** @see java.io.DataInput#readByte() */
    byte readByte() throws IOException;
}

/* */
