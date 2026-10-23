package com.sun.cldc.i18n.j2me;

import java.io.*;
import com.sun.cldc.i18n.*;

/** Shift-JIS (cp932) reader: i-appli default encoding. */
public class SJIS_Reader extends StreamReader {
    private int pending = -1;   /* a byte read ahead that was not a trail */

    private int next() throws IOException {
        if (pending >= 0) {
            int b = pending;
            pending = -1;
            return b;
        }
        return in.read();
    }

    synchronized public int read() throws IOException {
        int b = next();
        if (b < 0) return -1;
        if (b < 0x80) return b;
        if (b >= 0xA1 && b <= 0xDF) return 0xFF61 + b - 0xA1;
        if (SJIS_Table.row(b) < 0) return 0xFFFD;
        int t = next();
        if (t < 0) return 0xFFFD;
        if (SJIS_Table.column(t) < 0) {
            pending = t;
            return 0xFFFD;
        }
        return SJIS_Table.decode(b, t);
    }

    synchronized public int read(char cbuf[], int off, int len) throws IOException {
        int n = 0;
        while (n < len) {
            int c = read();
            if (c < 0) {
                return n == 0 ? -1 : n;
            }
            cbuf[off + n++] = (char) c;
        }
        return n;
    }

    public int sizeOf(byte[] array, int offset, int length) {
        int n = 0, end = offset + length;
        while (offset < end) {
            int b = array[offset++] & 0xFF;
            if (SJIS_Table.row(b) >= 0 && offset < end &&
                    SJIS_Table.column(array[offset] & 0xFF) >= 0) {
                offset++;
            }
            n++;
        }
        return n;
    }
}
