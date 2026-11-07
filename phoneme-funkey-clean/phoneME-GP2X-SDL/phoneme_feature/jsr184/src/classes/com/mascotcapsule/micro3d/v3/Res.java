package com.mascotcapsule.micro3d.v3;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

/** Reads a resource of the game's jar ("name" or "/name"). */
final class Res {
    private Res() {
    }

    static byte[] load(String name) throws IOException {
        if (name == null) {
            throw new NullPointerException();
        }
        String path = name.startsWith("/") ? name : "/" + name;
        /* no class literal: CLDC lacks Throwable.initCause (see audio notes) */
        InputStream in = "".getClass().getResourceAsStream(path);
        if (in == null) {
            throw new IOException("resource not found: " + name);
        }
        try {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[4096];
            int n;
            while ((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
            }
            return out.toByteArray();
        } finally {
            in.close();
        }
    }

    /** RuntimeException for bad data, as the handset throws */
    static RuntimeException bad(IOException e) {
        return new IllegalArgumentException(e.getMessage());
    }
}
