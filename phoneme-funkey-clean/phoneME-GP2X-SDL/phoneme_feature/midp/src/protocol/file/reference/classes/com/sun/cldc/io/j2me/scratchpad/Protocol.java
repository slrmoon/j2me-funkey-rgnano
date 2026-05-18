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
        stream.setPosition(SP_HEADER_SIZE + pos);
        this.mode = mode;
        remaining = length;
        open = true;
        System.out.println("DoJa scratchpad open pos=" + pos + " length=" + length);
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
        return new ScratchOutputStream();
    }

    public DataOutputStream openDataOutputStream() throws IOException {
        return new DataOutputStream(openOutputStream());
    }

    public void close() throws IOException {
        open = false;
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

    private final class ScratchInputStream extends InputStream {
        private boolean closed;

        public int read() throws IOException {
            byte[] one = new byte[1];
            int got = read(one, 0, 1);
            return got < 0 ? -1 : (one[0] & 0xff);
        }

        public int read(byte[] b, int off, int len) throws IOException {
            if (closed) {
                throw new IOException();
            }
            if (len == 0) {
                return 0;
            }
            return readBytes(b, off, len);
        }

        public void close() throws IOException {
            if (!closed) {
                closed = true;
                inputOpen = false;
                closeIfDone();
            }
        }
    }

    private final class ScratchOutputStream extends OutputStream {
        private boolean closed;

        public void write(int b) throws IOException {
            byte[] one = new byte[] { (byte)b };
            write(one, 0, 1);
        }

        public void write(byte[] b, int off, int len) throws IOException {
            if (closed) {
                throw new IOException();
            }
            writeBytes(b, off, len);
        }

        public void close() throws IOException {
            if (!closed) {
                closed = true;
                outputOpen = false;
                closeIfDone();
            }
        }
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
