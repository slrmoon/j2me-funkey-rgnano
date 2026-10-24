package com.nttdocomo.io;

import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;

/** Prints text to a Writer; errors are remembered instead of thrown. */
public class PrintWriter extends Writer {
    protected Writer out;
    private boolean autoFlush;
    private boolean error;

    protected PrintWriter() {
    }

    public PrintWriter(Writer out) {
        this(out, false);
    }

    public PrintWriter(Writer out, boolean autoFlush) {
        if (out == null) {
            throw new NullPointerException("out");
        }
        this.out = out;
        this.autoFlush = autoFlush;
    }

    public PrintWriter(OutputStream out) {
        this(out, false);
    }

    public PrintWriter(OutputStream out, boolean autoFlush) {
        this(new OutputStreamWriter(out), autoFlush);
    }

    public void flush() {
        try {
            if (out != null) {
                out.flush();
            }
        } catch (IOException e) {
            error = true;
        }
    }

    public void close() {
        try {
            if (out != null) {
                out.close();
            }
        } catch (IOException e) {
            error = true;
        }
        out = null;
    }

    public boolean checkError() {
        if (out != null) {
            flush();
        }
        return error;
    }

    protected void setError() {
        error = true;
    }

    public void write(int c) {
        try {
            if (out == null) {
                error = true;
                return;
            }
            out.write(c);
        } catch (IOException e) {
            error = true;
        }
    }

    public void write(char[] buf, int off, int len) {
        try {
            if (out == null) {
                error = true;
                return;
            }
            out.write(buf, off, len);
        } catch (IOException e) {
            error = true;
        }
    }

    public void write(char[] buf) {
        write(buf, 0, buf.length);
    }

    public void write(String s, int off, int len) {
        try {
            if (out == null) {
                error = true;
                return;
            }
            out.write(s, off, len);
        } catch (IOException e) {
            error = true;
        }
    }

    public void write(String s) {
        write(s, 0, s.length());
    }

    public void print(boolean b) { write(b ? "true" : "false"); }
    public void print(char c) { write(c); }
    public void print(int i) { write(String.valueOf(i)); }
    public void print(long l) { write(String.valueOf(l)); }
    public void print(char[] s) { write(s); }
    public void print(String s) { write(s == null ? "null" : s); }
    public void print(Object obj) { write(String.valueOf(obj)); }

    public void println() {
        write("\r\n");
        if (autoFlush) {
            flush();
        }
    }

    public void println(boolean b) { print(b); println(); }
    public void println(char c) { print(c); println(); }
    public void println(int i) { print(i); println(); }
    public void println(long l) { print(l); println(); }
    public void println(char[] s) { print(s); println(); }
    public void println(String s) { print(s); println(); }
    public void println(Object obj) { print(obj); println(); }
}
