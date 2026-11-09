package com.sun.cldc.i18n.j2me;

import java.io.*;
import com.sun.cldc.i18n.*;

/** Shift-JIS (cp932) writer; unmappable characters become '?'. */
public class SJIS_Writer extends StreamWriter {
    synchronized public void write(int c) throws IOException {
        int v = SJIS_Table.encode((char) c);
        if (v < 0) {
            out.write('?');
        } else if (v > 0xFF) {
            out.write(v >> 8);
            out.write(v & 0xFF);
        } else {
            out.write(v);
        }
    }

    synchronized public void write(char cbuf[], int off, int len) throws IOException {
        while (len-- > 0) {
            write(cbuf[off++]);
        }
    }

    synchronized public void write(String str, int off, int len) throws IOException {
        for (int i = 0; i < len; i++) {
            write(str.charAt(off + i));
        }
    }

    public int sizeOf(char[] array, int offset, int length) {
        int n = 0;
        for (int i = 0; i < length; i++) {
            n += SJIS_Table.encode(array[offset + i]) > 0xFF ? 2 : 1;
        }
        return n;
    }
}
