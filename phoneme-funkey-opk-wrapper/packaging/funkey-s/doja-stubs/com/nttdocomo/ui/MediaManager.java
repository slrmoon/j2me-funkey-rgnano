package com.nttdocomo.ui;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

import javax.microedition.io.Connector;

import com.funkey.doja.ResourceLoader;
import com.nttdocomo.io.ConnectionException;

public final class MediaManager {
    private MediaManager() {
    }

    public static MediaData getData(String name) {
        return new FkMediaData(name, null);
    }

    public static MediaData getData(InputStream in) {
        return new FkMediaData(null, readStream(in));
    }

    public static MediaData getData(byte[] data) {
        return new FkMediaData(null, copy(data));
    }

    public static MediaImage getImage(String name) {
        if (name == null) {
            throw new NullPointerException("name");
        }
        return new FkMediaImage(name, null);
    }

    public static MediaImage getImage(InputStream in) {
        return new FkMediaImage(null, readStream(in));
    }

    public static MediaImage getImage(byte[] data) {
        if (data == null) {
            throw new NullPointerException("data");
        }
        return new FkMediaImage(null, copy(data));
    }

    public static MediaImage getStreamingImage(String location, String contentType) {
        return new FkMediaImage(location, null);
    }

    public static MediaSound getSound(String name) {
        if (name == null) {
            throw new NullPointerException("name");
        }
        return new FkMediaSound(name, null);
    }

    public static MediaSound getSound(InputStream in) {
        return new FkMediaSound(null, readStream(in));
    }

    public static MediaSound getSound(byte[] data) {
        if (data == null) {
            throw new NullPointerException("data");
        }
        return new FkMediaSound(null, copy(data));
    }

    public static void use(MediaImage[] images, boolean exclusive) {
        for (int i = 0; images != null && i < images.length; i++) {
            try {
                images[i].use();
            } catch (Exception e) {
                System.out.println("DoJa: MediaManager.use: " + e);
            }
        }
    }

    public static void use(MediaSound[] sounds, boolean exclusive) {
        for (int i = 0; sounds != null && i < sounds.length; i++) {
            try {
                sounds[i].use();
            } catch (Exception e) {
                System.out.println("DoJa: MediaManager.use: " + e);
            }
        }
    }

    public static MediaImage createMediaImage(int width, int height) {
        FkMediaImage m = new FkMediaImage(null, null);
        m.image = Image.createImage(width, height);
        m.used = true;
        return m;
    }

    public static MediaSound createMediaSound(int size) {
        return new FkMediaSound(null, new byte[0]);
    }

    /* ------------------------------------------------------------------ */

    private static byte[] copy(byte[] d) {
        byte[] c = new byte[d.length];
        System.arraycopy(d, 0, c, 0, d.length);
        return c;
    }

    static byte[] readStream(InputStream in) {
        if (in == null) {
            throw new NullPointerException("inputStream");
        }
        try {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[1024];
            int n;
            while ((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
            }
            return out.toByteArray();
        } catch (IOException e) {
            throw new UIException(UIException.UNSUPPORTED_FORMAT, e.toString());
        }
    }

    /** Reads a media resource by DoJa location (resource:///, scratchpad:///, plain name). */
    static byte[] load(String name) throws ConnectionException {
        try {
            if (name.startsWith("scratchpad:")) {
                InputStream in = Connector.openInputStream(name);
                try {
                    return readScratchMedia(in);
                } finally {
                    in.close();
                }
            }
            if (name.startsWith("http:") || name.startsWith("https:")) {
                throw new ConnectionException(ConnectionException.NO_USE, "network: " + name);
            }
            return new ResourceLoader().read(name);
        } catch (ConnectionException e) {
            throw e;
        } catch (IOException e) {
            throw new ConnectionException(ConnectionException.NO_RESOURCES, name + ": " + e);
        }
    }

    /**
     * Scratchpad streams opened without ;length= run to the end of the
     * scratchpad; stop at the end of the media when the format tells it.
     */
    private static byte[] readScratchMedia(InputStream in) throws IOException {
        byte[] buf = new byte[8192];
        int len = 0, end = -1;
        while (true) {
            if (len == buf.length) {
                byte[] nb = new byte[buf.length * 2];
                System.arraycopy(buf, 0, nb, 0, len);
                buf = nb;
            }
            int n = in.read(buf, len, buf.length - len);
            if (n <= 0) {
                break;
            }
            len += n;
            end = mediaEnd(buf, len);
            if (end >= 0) {
                break;
            }
            if (end == -2) {
                end = -1;   /* unknown format: to the end of the scratchpad */
            }
        }
        if (end < 0 || end > len) {
            end = len;
        }
        byte[] out = new byte[end];
        System.arraycopy(buf, 0, out, 0, end);
        return out;
    }

    private static int u16be(byte[] b, int p) {
        return (b[p] & 255) << 8 | (b[p + 1] & 255);
    }

    private static int u32be(byte[] b, int p) {
        return (b[p] & 255) << 24 | (b[p + 1] & 255) << 16 | (b[p + 2] & 255) << 8 | (b[p + 3] & 255);
    }

    /**
     * Length of the media at the start of b when its end is already in the
     * first len bytes, -1 when more bytes are needed, -2 when the format
     * has no known end.
     */
    static int mediaEnd(byte[] b, int len) {
        if (len < 8) {
            return -1;
        }
        int b0 = b[0] & 255, b1 = b[1] & 255;
        if (b0 == 'm' && b1 == 'e' && b[2] == 'l' && b[3] == 'o') {
            return 8 + u32be(b, 4);
        }
        if (b0 == 'M' && b1 == 'M' && b[2] == 'M' && b[3] == 'D') {
            return 8 + u32be(b, 4);
        }
        if (b0 == 'R' && b1 == 'I' && b[2] == 'F' && b[3] == 'F') {
            return 8 + ((b[4] & 255) | (b[5] & 255) << 8 | (b[6] & 255) << 16 | (b[7] & 255) << 24);
        }
        if (b0 == 'G' && b1 == 'I' && b[2] == 'F') {
            return gifEnd(b, len);
        }
        if (b0 == 0x89 && b1 == 'P' && b[2] == 'N' && b[3] == 'G') {
            int p = 8;
            while (p + 12 <= len) {
                int n = u32be(b, p);
                if (n < 0) {
                    return -2;
                }
                if (b[p + 4] == 'I' && b[p + 5] == 'E' && b[p + 6] == 'N' && b[p + 7] == 'D') {
                    return p + 12;
                }
                p += 12 + n;
            }
            return -1;
        }
        if (b0 == 0xFF && b1 == 0xD8) {
            return jpegEnd(b, len);
        }
        return -2;
    }

    private static int gifEnd(byte[] b, int len) {
        if (len < 13) {
            return -1;
        }
        int p = 13;
        int flags = b[10] & 255;
        if ((flags & 0x80) != 0) {
            p += 3 << ((flags & 7) + 1);
        }
        while (p < len) {
            int t = b[p] & 255;
            if (t == 0x3B) {
                return p + 1;
            } else if (t == 0x21) {
                p += 2;   /* introducer, label, then sub-blocks */
            } else if (t == 0x2C) {
                if (p + 10 > len) {
                    return -1;
                }
                int lf = b[p + 9] & 255;
                p += 10;
                if ((lf & 0x80) != 0) {
                    p += 3 << ((lf & 7) + 1);
                }
                p += 1;   /* LZW minimum code size */
            } else {
                return -2;   /* broken: read everything */
            }
            while (true) {
                if (p >= len) {
                    return -1;
                }
                int n = b[p] & 255;
                p += 1 + n;
                if (n == 0) {
                    break;
                }
            }
        }
        return -1;
    }

    private static int jpegEnd(byte[] b, int len) {
        int p = 2;
        while (true) {
            while (p < len && (b[p] & 255) != 0xFF) {
                p++;
            }
            while (p < len && (b[p] & 255) == 0xFF) {
                p++;   /* fill bytes */
            }
            if (p >= len) {
                return -1;
            }
            int m = b[p] & 255;
            p++;
            if (m == 0xD9) {
                return p;
            }
            if (m == 0x01 || (m >= 0xD0 && m <= 0xD7)) {
                continue;   /* no length */
            }
            if (p + 2 > len) {
                return -1;
            }
            int n = u16be(b, p);
            p += n;
            if (m == 0xDA) {
                /* entropy-coded data: the next marker that is not a stuffed
                 * zero or a restart marker ends it */
                while (true) {
                    while (p < len && (b[p] & 255) != 0xFF) {
                        p++;
                    }
                    if (p + 1 >= len) {
                        return -1;
                    }
                    int q = b[p + 1] & 255;
                    if (q == 0 || (q >= 0xD0 && q <= 0xD7) || q == 0xFF) {
                        p += q == 0xFF ? 1 : 2;
                        continue;
                    }
                    break;
                }
            }
        }
    }

    /* ------------------------------------------------------------------ */

    abstract static class Res implements MediaResource {
        String name;
        byte[] data;
        boolean used;
        boolean disposed;
        java.util.Hashtable props;

        Res(String n, byte[] d) {
            name = n;
            data = d;
        }

        byte[] bytes() throws ConnectionException {
            if (data == null && name != null) {
                data = load(name);
            }
            return data;
        }

        public void use() throws ConnectionException {
            if (disposed) {
                throw new UIException(UIException.ILLEGAL_STATE, "disposed");
            }
            if (!used) {
                prepare();
                used = true;
            }
        }

        public void use(MediaResource other, boolean exclusive) throws ConnectionException {
            use();
        }

        abstract void prepare() throws ConnectionException;

        public void unuse() {
            used = false;
        }

        public void dispose() {
            used = false;
            disposed = true;
            data = null;
        }

        public String getProperty(String key) {
            return props == null ? null : (String) props.get(key);
        }

        public void setProperty(String key, String value) {
            if (props == null) {
                props = new java.util.Hashtable();
            }
            if (value == null) {
                props.remove(key);
            } else {
                props.put(key, value);
            }
        }

        public boolean isRedistributable() {
            return true;
        }

        public boolean setRedistributable(boolean r) {
            return true;
        }
    }

    static final class FkMediaImage extends Res implements MediaImage {
        Image image;
        private ExifData exif;

        FkMediaImage(String n, byte[] d) {
            super(n, d);
        }

        void prepare() throws ConnectionException {
            if (image != null) {
                return;
            }
            byte[] b = bytes();
            long t0 = com.funkey.doja.DoJaRuntime.TRACE ? System.currentTimeMillis() : 0;
            try {
                image = Image.decode(b);
                if (com.funkey.doja.DoJaRuntime.TRACE) {
                    System.out.println("DoJa trace: image " + name + " " + b.length + " bytes "
                            + image.getWidth() + "x" + image.getHeight() + " "
                            + (System.currentTimeMillis() - t0) + " ms");
                }
            } catch (IOException e) {
                throw new UIException(UIException.UNSUPPORTED_FORMAT, (name == null ? "image" : name) + ": " + e);
            } catch (IllegalArgumentException e) {
                throw new UIException(UIException.UNSUPPORTED_FORMAT, (name == null ? "image" : name) + ": " + e);
            }
            if (name != null) {
                data = null; /* decoded: the bytes can be reloaded */
            }
        }

        public void unuse() {
            super.unuse();
        }

        public void dispose() {
            super.dispose();
            if (image != null) {
                image.dispose();
            }
            image = null;
        }

        private void ensure() {
            if (image == null && !disposed) {
                try {
                    use();
                } catch (ConnectionException e) {
                    throw new UIException(UIException.ILLEGAL_STATE, e.toString());
                }
            }
        }

        public int getWidth() {
            ensure();
            return image == null ? 0 : image.getWidth();
        }

        public int getHeight() {
            ensure();
            return image == null ? 0 : image.getHeight();
        }

        public Image getImage() {
            if (disposed) {
                throw new UIException(UIException.ILLEGAL_STATE, "disposed");
            }
            ensure();
            return image;
        }

        public ExifData getExifData() {
            return exif;
        }

        public void setExifData(ExifData e) {
            exif = e;
        }
    }

    static final class FkMediaSound extends Res implements MediaSound {
        byte[] midi;
        byte[] wav;
        String type;

        FkMediaSound(String n, byte[] d) {
            super(n, d);
        }

        void prepare() throws ConnectionException {
            byte[] b = bytes();
            if (b == null || b.length < 4) {
                type = "unknown";
                return;
            }
            if (b[0] == 'm' && b[1] == 'e' && b[2] == 'l' && b[3] == 'o') {
                type = "audio/mld";
            } else if (b[0] == 'M' && b[1] == 'T' && b[2] == 'h' && b[3] == 'd') {
                type = "audio/midi";
            } else if (b[0] == 'R' && b[1] == 'I' && b[2] == 'F' && b[3] == 'F') {
                type = "audio/x-wav";
            } else if (b[0] == 'M' && b[1] == 'M' && b[2] == 'M' && b[3] == 'D') {
                type = "application/vnd.smaf";
            } else {
                type = "unknown";
            }
        }

        public void dispose() {
            super.dispose();
            midi = null;
            wav = null;
        }
    }

    static final class FkMediaData extends Res implements MediaData {
        FkMediaData(String n, byte[] d) {
            super(n, d);
        }

        void prepare() throws ConnectionException {
            bytes();
        }
    }
}
