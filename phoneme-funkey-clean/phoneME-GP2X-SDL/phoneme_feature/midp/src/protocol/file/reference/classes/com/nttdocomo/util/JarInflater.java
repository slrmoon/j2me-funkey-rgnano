package com.nttdocomo.util;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

import com.sun.midp.installer.JarReader;
import com.sun.midp.io.j2me.storage.RandomAccessStream;

public class JarInflater {
    private static int nextId;

    private String tempJarPath;

    public JarInflater() {
    }

    public JarInflater(InputStream in) throws IOException {
        tempJarPath = makeTempPath();
        writeTempJar(in, tempJarPath);
        System.out.println("DoJa JarInflater temp=" + tempJarPath);
    }

    public byte[] getResource(String name) throws IOException {
        if (tempJarPath == null) {
            throw new IOException(name);
        }
        return readJarEntry(name);
    }

    public InputStream getInputStream(String name) throws IOException {
        return new ByteArrayInputStream(getResource(name));
    }

    public void close() throws IOException {
        tempJarPath = null;
    }

    private byte[] readJarEntry(String name) throws IOException {
        byte[] data = JarReader.readJarEntry(tempJarPath, name);
        if (data == null) {
            throw new IOException(name);
        }
        return data;
    }

    private static String makeTempPath() {
        String scratch = System.getProperty("doja.scratchpad.path");
        int slash;

        if (scratch == null || scratch.length() == 0) {
            scratch = "/mnt/FunKey/.doja/scratchpad/doja.sp";
        }

        slash = scratch.lastIndexOf('/');
        if (slash >= 0) {
            scratch = scratch.substring(0, slash + 1);
        } else {
            scratch = "";
        }

        return scratch + "jarinflater-" + System.currentTimeMillis() + "-" +
                (nextId++) + ".jar";
    }

    private static void writeTempJar(InputStream in, String path)
            throws IOException {
        RandomAccessStream out = new RandomAccessStream();
        byte[] buf = new byte[4096];
        int got;

        out.connect(path, javax.microedition.io.Connector.READ_WRITE);
        try {
            while ((got = in.read(buf)) >= 0) {
                if (got > 0) {
                    out.writeBytes(buf, 0, got);
                }
            }
            out.commitWrite();
        } finally {
            out.disconnect();
        }
    }
}
