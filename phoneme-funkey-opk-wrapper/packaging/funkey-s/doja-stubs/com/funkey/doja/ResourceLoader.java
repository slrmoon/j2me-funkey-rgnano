package com.funkey.doja;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public final class ResourceLoader {
    public byte[] read(String name) throws IOException {
        InputStream in = open(name);
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buf = new byte[512];
        int n;
        while ((n = in.read(buf)) >= 0) {
            out.write(buf, 0, n);
        }
        in.close();
        return out.toByteArray();
    }

    public InputStream open(String name) throws IOException {
        String path = normalize(name);
        InputStream in = getClass().getResourceAsStream(path);
        if (in == null && path.startsWith("/")) {
            in = getClass().getResourceAsStream(path.substring(1));
        }
        if (in == null) {
            throw new IOException("Resource not found: " + name);
        }
        return in;
    }

    public static String normalize(String name) {
        String path = name == null ? "" : name;
        if (path.startsWith("resource:///")) {
            path = path.substring(11);
        } else if (path.startsWith("resource://")) {
            path = path.substring(10);
        }
        if (!path.startsWith("/")) {
            path = "/" + path;
        }
        return path;
    }
}
