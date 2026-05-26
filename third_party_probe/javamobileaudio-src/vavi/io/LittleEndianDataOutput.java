/*
 * Copyright (c) 2004 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.io;

import java.io.DataOutput;
import java.io.IOException;


/**
 * LittleEndianDataOutput.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 040929 nsano initial version <br>
 */
public interface LittleEndianDataOutput extends DataOutput {
    /** @see java.io.DataOutput#writeDouble(double) */
    void writeDouble(double arg0) throws IOException;

    /** @see java.io.DataOutput#writeFloat(float) */
    void writeFloat(float arg0) throws IOException;

    /** @see java.io.DataOutput#writeByte(int) */
    void writeByte(int b) throws IOException;

    /** @see java.io.DataOutput#writeChar(int) */
    void writeChar(int arg0) throws IOException;

    /** @see java.io.DataOutput#writeInt(int) */
    void writeInt(int i) throws IOException;

    /** @see java.io.DataOutput#writeShort(int) */
    void writeShort(int i) throws IOException;

    /** @see java.io.DataOutput#writeLong(long) */
    void writeLong(long l) throws IOException;

    /** @see java.io.DataOutput#writeBoolean(boolean) */
    void writeBoolean(boolean arg0) throws IOException;

    /** @see java.io.DataOutput#writeBytes(java.lang.String) */
    void writeBytes(String arg0) throws IOException;

    /** @see java.io.DataOutput#writeChars(java.lang.String) */
    void writeChars(String arg0) throws IOException;

    /** @see java.io.DataOutput#writeUTF(java.lang.String) */
    void writeUTF(String arg0) throws IOException;
}

/* */
