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
        tempJarPath = makeCachePath(nextId++);
        if (cacheExists(tempJarPath)) {
            System.out.println("DoJa JarInflater cache=" + tempJarPath);
        } else {
            writeTempJar(in, tempJarPath);
            System.out.println("DoJa JarInflater cache write=" + tempJarPath);
        }
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

    private static String makeCachePath(int id) {
        String scratch = System.getProperty("doja.scratchpad.path");
        int slash;
        String prefix = "doja";

        if (scratch == null || scratch.length() == 0) {
            scratch = "/mnt/FunKey/.doja/scratchpad/doja.sp";
        }

        slash = scratch.lastIndexOf('/');
        if (slash >= 0) {
            prefix = scratch.substring(slash + 1);
            scratch = scratch.substring(0, slash + 1);
        } else {
            prefix = scratch;
            scratch = "";
        }

        if (prefix.endsWith(".sp")) {
            prefix = prefix.substring(0, prefix.length() - 3);
        }

        return scratch + "jarinflater-" + sanitize(prefix) + "-" + id + ".jar";
    }

    private static String sanitize(String value) {
        StringBuffer out = new StringBuffer(value.length());

        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_' || c == '-' ||
                    c == '.') {
                out.append(c);
            } else {
                out.append('_');
            }
        }

        if (out.length() == 0) {
            return "doja";
        }
        return out.toString();
    }

    private static boolean cacheExists(String path) {
        try {
            return new com.sun.midp.io.j2me.storage.File().exists(path);
        } catch (Throwable ignored) {
            return false;
        }
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
