package com.nttdocomo.system;

import java.io.IOException;

import javax.microedition.io.Connector;

import com.sun.midp.io.j2me.storage.RandomAccessStream;

/**
 * Whole-file reads for the i-appli runtime (ImageStore pictures): the
 * storage classes are hidden from application code.
 */
public final class FkFiles {
    private FkFiles() {
    }

    /** the file's bytes, or null when it cannot be read */
    public static byte[] read(String path) {
        RandomAccessStream s = null;
        try {
            s = new RandomAccessStream();
            s.connect(path, Connector.READ);
            int n = s.getSizeOf();
            byte[] b = new byte[n];
            int off = 0;
            while (off < n) {
                int r = s.readBytes(b, off, n - off);
                if (r <= 0) {
                    break;
                }
                off += r;
            }
            return b;
        } catch (IOException e) {
            return null;
        } catch (RuntimeException e) {
            return null;
        } finally {
            if (s != null) {
                try {
                    s.disconnect();
                } catch (IOException e) {
                }
            }
        }
    }
}
