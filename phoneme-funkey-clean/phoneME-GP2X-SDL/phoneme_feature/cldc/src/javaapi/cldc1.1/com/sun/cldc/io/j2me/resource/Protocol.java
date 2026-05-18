/*
 * Resource protocol for DoJa/i-appli resource:/// URLs.
 */

package com.sun.cldc.io.j2me.resource;

import java.io.*;
import javax.microedition.io.*;
import com.sun.cldc.io.*;

public class Protocol implements ConnectionBaseInterface, InputConnection {
    private String name;
    private boolean open;

    public Connection openPrim(String name, int mode, boolean timeouts)
            throws IOException {
        this.name = normalize(name);
        this.open = true;
        return this;
    }

    public InputStream openInputStream() throws IOException {
        ensureOpen();
        return new ResourceInputStream(name);
    }

    public DataInputStream openDataInputStream() throws IOException {
        return new DataInputStream(openInputStream());
    }

    public void close() throws IOException {
        open = false;
    }

    private void ensureOpen() throws IOException {
        if (!open) {
            throw new IOException();
        }
    }

    private static String normalize(String name) {
        String path = name == null ? "" : name;
        while (path.startsWith("/")) {
            path = path.substring(1);
        }
        return path;
    }
}
