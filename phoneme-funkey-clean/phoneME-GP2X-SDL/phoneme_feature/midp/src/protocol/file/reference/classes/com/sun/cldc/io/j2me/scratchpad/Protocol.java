package com.sun.cldc.io.j2me.scratchpad;

import java.io.*;
import javax.microedition.io.*;

import com.sun.cldc.io.ConnectionBaseInterface;
import com.sun.midp.io.j2me.storage.RandomAccessStream;

public class Protocol implements ConnectionBaseInterface, StreamConnection {
    private static final int SP_HEADER_SIZE = 64;

    private RandomAccessStream stream;
    private boolean open;
    private boolean inputOpen;
    private boolean outputOpen;
    private int remaining;
    private int mode;
    private ScratchOutputStream out;

    private static final boolean TRACE = "1".equals(System.getProperty("doja.trace"));

    public Connection openPrim(String name, int mode, boolean timeouts)
            throws IOException {
        String path = System.getProperty("doja.scratchpad.path");
        int pos = parseIntParam(name, "pos", 0);
        int length = parseIntParam(name, "length", -1);

        if (path == null || path.length() == 0) {
            throw new ConnectionNotFoundException("scratchpad path missing");
        }
        if (mode != Connector.READ && mode != Connector.WRITE &&
                mode != Connector.READ_WRITE) {
            throw new IllegalArgumentException("mode");
        }

        stream = new RandomAccessStream();
        stream.connect(path, mode == Connector.READ ? Connector.READ : Connector.READ_WRITE);
        /*
         * DoJa 4+ can declare several scratchpads (SPsize = a,b,c), addressed
         * as scratchpad:///N. The .sp header holds their sizes: 16
         * little-endian ints, -1 after the last one. Segment N starts after
         * the previous ones and a stream does not run past its segment.
         */
        int index = parseIndex(name);
        int base = 0;
        int segSize = -1;
        byte[] hdr = new byte[SP_HEADER_SIZE];
        int got = 0;
        stream.setPosition(0);
        while (got < hdr.length) {
            int n = stream.readBytes(hdr, got, hdr.length - got);
            if (n <= 0) {
                break;
            }
            got += n;
        }
        if (got < SP_HEADER_SIZE || headerSize(hdr, 0) <= 0) {
            /* new scratchpad: sizes from the .jam (launcher: -Ddoja.spsize) */
            String sizes = System.getProperty("doja.spsize");
            if (sizes != null && sizes.length() > 0) {
                for (int i = 0; i < SP_HEADER_SIZE; i++) {
                    hdr[i] = (byte) 0xff;
                }
                int k = 0, v = 0;
                boolean any = false;
                for (int i = 0; i <= sizes.length() && k < 16; i++) {
                    char ch = i < sizes.length() ? sizes.charAt(i) : ',';
                    if (ch >= '0' && ch <= '9') {
                        v = v * 10 + (ch - '0');
                        any = true;
                    } else if (ch == ',') {
                        if (any) {
                            hdr[k * 4] = (byte) v;
                            hdr[k * 4 + 1] = (byte) (v >> 8);
                            hdr[k * 4 + 2] = (byte) (v >> 16);
                            hdr[k * 4 + 3] = (byte) (v >> 24);
                            k++;
                        }
                        v = 0;
                        any = false;
                    }
                }
                if (k > 0) {
                    got = SP_HEADER_SIZE;
                    if (mode != Connector.READ) {
                        stream.setPosition(0);
                        stream.writeBytes(hdr, 0, SP_HEADER_SIZE);
                        stream.commitWrite();
                    }
                }
            }
        }
        if (got == SP_HEADER_SIZE && headerSize(hdr, 0) > 0) {
            for (int i = 0; i < index && i < 16; i++) {
                int sz = headerSize(hdr, i);
                if (sz <= 0) {
                    break;
                }
                base += sz;
            }
            segSize = index < 16 ? headerSize(hdr, index) : -1;
            if (segSize > 0 && pos <= segSize && (length < 0 || pos + length > segSize)) {
                length = segSize - pos;
            }
        }
        stream.setPosition(SP_HEADER_SIZE + base + pos);
        this.mode = mode;
        remaining = length;
        open = true;
        if (TRACE) {
            System.out.println("DoJa scratchpad open " + name + " base=" + base + " pos=" + pos + " length=" + length);
        }
        return this;
    }

    public InputStream openInputStream() throws IOException {
        ensureOpen();
        if (mode == Connector.WRITE || inputOpen) {
            throw new IOException();
        }
        inputOpen = true;
        return new ScratchInputStream();
    }

    public DataInputStream openDataInputStream() throws IOException {
        return new DataInputStream(openInputStream());
    }

    public OutputStream openOutputStream() throws IOException {
        ensureOpen();
        if (mode == Connector.READ || outputOpen) {
            throw new IOException();
        }
        outputOpen = true;
        out = new ScratchOutputStream();
        return out;
    }

    public DataOutputStream openDataOutputStream() throws IOException {
        return new DataOutputStream(openOutputStream());
    }

    public void close() throws IOException {
        open = false;
        if (out != null && outputOpen) {
            out.flush();   /* the stream may never be closed */
        }
        closeIfDone();
    }

    private void ensureOpen() throws IOException {
        if (!open || stream == null) {
            throw new IOException();
        }
    }

    private void closeIfDone() throws IOException {
        if (!inputOpen && !outputOpen && stream != null) {
            open = false;
            stream.disconnect();
            stream = null;
        }
    }

    private int readBytes(byte[] b, int off, int len) throws IOException {
        if (remaining == 0) {
            return -1;
        }
        if (remaining > 0 && len > remaining) {
            len = remaining;
        }
        int got = stream.readBytes(b, off, len);
        if (got > 0 && remaining > 0) {
            remaining -= got;
        }
        return got;
    }

    private void writeBytes(byte[] b, int off, int len) throws IOException {
        if (remaining == 0) {
            return;
        }
        if (remaining > 0 && len > remaining) {
            len = remaining;
        }
        if (len <= 0) {
            return;
        }
        stream.writeBytes(b, off, len);
        stream.commitWrite();
        if (remaining > 0) {
            remaining -= len;
        }
    }

    /*
     * Buffered: games read their data a few bytes at a time (DataInputStream)
     * and every native read is a file seek + read. Read-only connections
     * only, a read-write one shares the file position with its output.
     */
    private final class ScratchInputStream extends InputStream {
        private boolean closed;
        private final byte[] buf = mode == Connector.READ ? new byte[4096] : null;
        private int bufPos, bufLen;

        private boolean fill() throws IOException {
            int got = readBytes(buf, 0, buf.length);
            bufPos = 0;
            bufLen = got < 0 ? 0 : got;
            return got > 0;
        }

        public int read() throws IOException {
            if (closed) {
                throw new IOException();
            }
            if (buf != null) {
                if (bufPos >= bufLen && !fill()) {
                    return -1;
                }
                return buf[bufPos++] & 0xff;
            }
            byte[] one = new byte[1];
            int got = readBytes(one, 0, 1);
            return got <= 0 ? -1 : (one[0] & 0xff);
        }

        public int read(byte[] b, int off, int len) throws IOException {
            if (closed) {
                throw new IOException();
            }
            if (len == 0) {
                return 0;
            }
            if (buf == null) {
                return readBytes(b, off, len);
            }
            int done = 0;
            if (bufPos < bufLen) {
                done = Math.min(len, bufLen - bufPos);
                System.arraycopy(buf, bufPos, b, off, done);
                bufPos += done;
                if (done == len) {
                    return done;
                }
            }
            if (len - done >= buf.length) {
                int got = readBytes(b, off + done, len - done);
                return got <= 0 ? (done > 0 ? done : -1) : done + got;
            }
            if (!fill()) {
                return done > 0 ? done : -1;
            }
            int n = Math.min(len - done, bufLen);
            System.arraycopy(buf, 0, b, off + done, n);
            bufPos = n;
            return done + n;
        }

        public int available() throws IOException {
            return buf == null ? 0 : bufLen - bufPos;
        }

        public void close() throws IOException {
            if (!closed) {
                closed = true;
                inputOpen = false;
                closeIfDone();
            }
        }
    }

    /* Buffered, committed on flush/close (a commit per byte made saving
     * through DataOutputStream crawl). */
    private final class ScratchOutputStream extends OutputStream {
        private boolean closed;
        private final byte[] buf = new byte[4096];
        private int len;

        public void write(int b) throws IOException {
            if (closed) {
                throw new IOException();
            }
            if (len == buf.length) {
                flush();
            }
            buf[len++] = (byte) b;
        }

        public void write(byte[] b, int off, int n) throws IOException {
            if (closed) {
                throw new IOException();
            }
            if (n > buf.length - len) {
                flush();
                if (n >= buf.length) {
                    writeBytes(b, off, n);
                    return;
                }
            }
            System.arraycopy(b, off, buf, len, n);
            len += n;
        }

        public void flush() throws IOException {
            if (len > 0) {
                int n = len;
                len = 0;
                writeBytes(buf, 0, n);
            }
        }

        public void close() throws IOException {
            if (!closed) {
                try {
                    flush();
                } finally {
                    closed = true;
                    outputOpen = false;
                    closeIfDone();
                }
            }
        }
    }

    private static int headerSize(byte[] h, int i) {
        int o = i * 4;
        return (h[o] & 255) | (h[o + 1] & 255) << 8 | (h[o + 2] & 255) << 16 | (h[o + 3] & 255) << 24;
    }

    /** "///N;pos=..." -> N (0 when missing). */
    private static int parseIndex(String name) {
        if (name == null) {
            return 0;
        }
        int i = 0;
        while (i < name.length() && name.charAt(i) == '/') {
            i++;
        }
        int v = 0;
        boolean any = false;
        while (i < name.length() && name.charAt(i) >= '0' && name.charAt(i) <= '9') {
            v = v * 10 + (name.charAt(i) - '0');
            i++;
            any = true;
        }
        return any ? v : 0;
    }

    private static int parseIntParam(String name, String key, int fallback) {
        String needle = key + "=";
        int start = name == null ? -1 : name.indexOf(needle);
        int end;
        if (start < 0) {
            return fallback;
        }
        start += needle.length();
        end = start;
        while (end < name.length()) {
            char ch = name.charAt(end);
            if (ch < '0' || ch > '9') {
                break;
            }
            end++;
        }
        if (end == start) {
            return fallback;
        }
        try {
            return Integer.parseInt(name.substring(start, end));
        } catch (NumberFormatException e) {
            return fallback;
        }
    }
}
