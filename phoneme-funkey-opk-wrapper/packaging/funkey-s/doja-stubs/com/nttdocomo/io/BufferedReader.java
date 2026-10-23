package com.nttdocomo.io;

import java.io.IOException;
import java.io.Reader;

public class BufferedReader extends Reader {
    private Reader in;
    private char[] buf;
    private int pos, len;
    private int markPos = -1;
    private boolean skipLF;

    public BufferedReader(Reader in) {
        this(in, 1024);
    }

    public BufferedReader(Reader in, int size) {
        if (in == null) {
            throw new NullPointerException("in");
        }
        if (size <= 0) {
            throw new IllegalArgumentException("size");
        }
        this.in = in;
        buf = new char[size];
    }

    protected BufferedReader() {
    }

    private void ensureOpen() throws IOException {
        if (in == null) {
            throw new IOException("closed");
        }
    }

    private boolean fill() throws IOException {
        if (markPos >= 0) {
            if (len == buf.length) {
                char[] nb = new char[buf.length * 2];
                System.arraycopy(buf, 0, nb, 0, len);
                buf = nb;
            }
        } else {
            pos = 0;
            len = 0;
        }
        int n = in.read(buf, len, buf.length - len);
        if (n <= 0) {
            return false;
        }
        len += n;
        return true;
    }

    public int read() throws IOException {
        ensureOpen();
        if (pos >= len && !fill()) {
            return -1;
        }
        return buf[pos++];
    }

    public int read(char[] b, int off, int n) throws IOException {
        ensureOpen();
        if (n == 0) {
            return 0;
        }
        if (pos >= len && !fill()) {
            return -1;
        }
        int k = Math.min(n, len - pos);
        System.arraycopy(buf, pos, b, off, k);
        pos += k;
        return k;
    }

    public String readLine() throws IOException {
        ensureOpen();
        StringBuffer sb = null;
        while (true) {
            if (pos >= len && !fill()) {
                return sb == null ? null : sb.toString();
            }
            char c = buf[pos++];
            if (skipLF) {
                skipLF = false;
                if (c == '\n') {
                    continue;
                }
            }
            if (c == '\n') {
                return sb == null ? "" : sb.toString();
            }
            if (c == '\r') {
                skipLF = true;
                return sb == null ? "" : sb.toString();
            }
            if (sb == null) {
                sb = new StringBuffer();
            }
            sb.append(c);
        }
    }

    public long skip(long n) throws IOException {
        long k = 0;
        while (k < n && read() >= 0) {
            k++;
        }
        return k;
    }

    public boolean ready() throws IOException {
        ensureOpen();
        return pos < len || in.ready();
    }

    public boolean markSupported() {
        return true;
    }

    public void mark(int readAheadLimit) throws IOException {
        ensureOpen();
        if (pos > 0) {
            System.arraycopy(buf, pos, buf, 0, len - pos);
            len -= pos;
            pos = 0;
        }
        markPos = 0;
    }

    public void reset() throws IOException {
        ensureOpen();
        if (markPos < 0) {
            throw new IOException("not marked");
        }
        pos = markPos;
    }

    public void close() throws IOException {
        if (in != null) {
            in.close();
            in = null;
        }
    }
}
