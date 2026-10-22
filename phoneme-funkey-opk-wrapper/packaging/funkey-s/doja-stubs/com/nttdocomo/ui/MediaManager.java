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
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] head = new byte[8];
        int got = 0;
        while (got < 8) {
            int n = in.read(head, got, 8 - got);
            if (n < 0) {
                break;
            }
            got += n;
        }
        out.write(head, 0, got);
        long limit = Long.MAX_VALUE;
        if (got == 8 && head[0] == 'm' && head[1] == 'e' && head[2] == 'l' && head[3] == 'o') {
            limit = (((head[4] & 255L) << 24) | ((head[5] & 255) << 16)
                    | ((head[6] & 255) << 8) | (head[7] & 255));
        }
        byte[] buf = new byte[1024];
        long total = 0;
        while (total < limit) {
            int want = buf.length;
            if (limit - total < want) {
                want = (int) (limit - total);
            }
            int n = in.read(buf, 0, want);
            if (n <= 0) {
                break;
            }
            out.write(buf, 0, n);
            total += n;
        }
        return out.toByteArray();
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
            try {
                image = Image.decode(b);
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
