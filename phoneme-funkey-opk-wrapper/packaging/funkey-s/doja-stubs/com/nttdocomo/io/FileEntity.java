package com.nttdocomo.io;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import com.funkey.doja.FsStore;

/** An open file: contents in memory, written back on flush/close. */
public class FileEntity {
    String path;
    byte[] data;
    int size;
    boolean readable, writable, dirty, closed;
    private int bufferSize = 512;

    protected FileEntity() {
    }

    /** runtime: com.nttdocomo.fs.File.open */
    public static FileEntity fkOpen(String path, boolean readable, boolean writable) throws IOException {
        FileEntity e = new FileEntity();
        e.path = path;
        byte[] d = FsStore.read(path);
        e.data = d == null ? new byte[0] : d;
        e.size = e.data.length;
        e.readable = readable;
        e.writable = writable;
        return e;
    }

    void check() throws IOException {
        if (closed) {
            throw new IOException("closed");
        }
    }

    void ensure(int n) {
        if (n > data.length) {
            byte[] nd = new byte[Math.max(n, data.length * 2 + 64)];
            System.arraycopy(data, 0, nd, 0, size);
            data = nd;
        }
    }

    void flushData() throws IOException {
        if (dirty) {
            FsStore.write(path, data, size);
            dirty = false;
        }
    }

    public synchronized void close() throws IOException {
        if (!closed) {
            flushData();
            closed = true;
        }
    }

    public void setBufferSize(int bufferSize) throws IOException {
        check();
        if (bufferSize <= 0) {
            throw new IllegalArgumentException("bufferSize");
        }
        this.bufferSize = bufferSize;
    }

    public int getBufferSize() throws IOException {
        check();
        return bufferSize;
    }

    public FileDataInput openDataInput() throws IOException {
        check();
        if (!readable) {
            throw new IOException("write only");
        }
        return new FileDataInput(this);
    }

    public FileDataOutput openDataOutput() throws IOException {
        check();
        if (!writable) {
            throw new IOException("read only");
        }
        return new FileDataOutput(this);
    }

    public InputStream openInputStream() throws IOException {
        check();
        if (!readable) {
            throw new IOException("write only");
        }
        return new ByteArrayInputStream(data, 0, size);
    }

    public OutputStream openOutputStream() throws IOException {
        final FileDataOutput o = openDataOutput();
        return new OutputStream() {
            public void write(int b) throws IOException {
                o.write(b);
            }

            public void write(byte[] b, int off, int len) throws IOException {
                o.write(b, off, len);
            }

            public void flush() throws IOException {
                o.flush();
            }

            public void close() throws IOException {
                o.close();
            }
        };
    }
}
