package com.nttdocomo.system;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

import com.nttdocomo.ui.MediaImage;
import com.nttdocomo.ui.MediaManager;

/**
 * The handset picture folder ("My Picture"). Here it is a folder next to
 * the game (the launcher passes -Ddoja.pictures): entry n is the file
 * Image%08X.gif (or .jpg / .png), as the DoJa SDK names them. Some
 * games keep data packs there (FF7 Dirge of Cerberus Lost Episode).
 * selectEntry() hands out the entries in order, as a player picking file
 * 1, 2, 3... would.
 */
public final class ImageStore {
    public static final boolean isSupportedAddImageArray = true;

    private final int id;
    private final byte[] data;
    private static int nextPick = 1;

    protected ImageStore() {
        id = 0;
        data = new byte[0];
    }

    private ImageStore(int id, byte[] data) {
        this.id = id;
        this.data = data;
    }

    private static String folder() {
        String d = System.getProperty("doja.pictures");
        return d == null || d.length() == 0 ? null : d;
    }

    private static String number(int id) {
        /* the SDK numbers them in hex: Image0000000A.gif is entry 10 */
        String s = Integer.toHexString(id).toUpperCase();
        while (s.length() < 8) {
            s = "0" + s;
        }
        return s;
    }

    private static final String[] EXT = { ".gif", ".jpg", ".png", ".GIF", ".JPG", ".PNG" };

    private static byte[] read(int id) {
        String dir = folder();
        if (dir == null || id <= 0) {
            return null;
        }
        for (int e = 0; e < EXT.length; e++) {
            byte[] b = readFile(dir + "/Image" + number(id) + EXT[e]);
            if (b != null) {
                return b;
            }
        }
        return null;
    }

    private static byte[] readFile(String path) {
        return FkFiles.read(path);
    }

    public static ImageStore selectEntry() throws InterruptedOperationException {
        int id = selectEntryId();
        try {
            return getEntry(id);
        } catch (StoreException e) {
            throw new InterruptedOperationException("no picture");
        }
    }

    public static int selectEntryId() throws InterruptedOperationException {
        if (TRACE) {
            System.out.println("DoJa trace: ImageStore.selectEntry -> " + nextPick);
        }
        if (read(nextPick) == null) {
            nextPick = 1;
            if (read(1) == null) {
                System.out.println("DoJa: ImageStore: no pictures (folder " + folder() + ")");
                throw new InterruptedOperationException("no picture");
            }
        }
        return nextPick++;
    }

    private static final boolean TRACE = "1".equals(System.getProperty("doja.trace"));

    public static ImageStore getEntry(int id) throws StoreException {
        byte[] b = read(id);
        if (TRACE) {
            System.out.println("DoJa trace: ImageStore.getEntry " + id + " -> "
                    + (b == null ? "none" : b.length + " bytes"));
        }
        if (b == null) {
            throw new StoreException(StoreException.NOT_FOUND, "picture " + id);
        }
        return new ImageStore(id, b);
    }

    public static int[] getEntryIds(int folderId) throws StoreException {
        int n = 0;
        while (n < 9999 && read(n + 1) != null) {
            n++;
        }
        if (n == 0) {
            return null;
        }
        int[] ids = new int[n];
        for (int i = 0; i < n; i++) {
            ids[i] = i + 1;
        }
        return ids;
    }

    public static int addEntry(MediaImage image, boolean exclusive) throws InterruptedOperationException {
        throw new InterruptedOperationException("read only");
    }

    public static int addEntry(MediaImage image) throws InterruptedOperationException {
        throw new InterruptedOperationException("read only");
    }

    public static int[] addEntry(MediaImage[] images) throws InterruptedOperationException {
        throw new InterruptedOperationException("read only");
    }

    public final int getId() {
        return id;
    }

    public final MediaImage getImage() {
        return MediaManager.getImage(data);
    }

    public final InputStream getInputStream() {
        return new ByteArrayInputStream(data);
    }
}
