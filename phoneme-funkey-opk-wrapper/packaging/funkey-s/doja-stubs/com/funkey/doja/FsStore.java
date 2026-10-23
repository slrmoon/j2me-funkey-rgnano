package com.funkey.doja;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import javax.microedition.rms.RecordStore;

/**
 * Backing store of the DoJa file system API (com.nttdocomo.fs): one RMS
 * record store per file, record 1 = UTF path + contents. Paths are
 * "<device>|<access>|<share>/<file name>".
 */
public final class FsStore {
    private FsStore() {
    }

    private static String storeName(String path) {
        return "dfs" + Integer.toHexString(path.hashCode()) + Integer.toHexString(path.length());
    }

    private static RecordStore open(String path, boolean create) {
        try {
            return RecordStore.openRecordStore(storeName(path), create);
        } catch (Exception e) {
            return null;
        }
    }

    private static void close(RecordStore rs) {
        try {
            rs.closeRecordStore();
        } catch (Exception e) {
        }
    }

    public static boolean exists(String path) {
        RecordStore rs = open(path, false);
        if (rs == null) {
            return false;
        }
        close(rs);
        return true;
    }

    /** Contents, null when the file does not exist. */
    public static byte[] read(String path) {
        RecordStore rs = open(path, false);
        if (rs == null) {
            return null;
        }
        try {
            byte[] r = rs.getRecord(1);
            int n = ((r[0] & 255) << 8) | (r[1] & 255);
            byte[] d = new byte[r.length - 2 - n];
            System.arraycopy(r, 2 + n, d, 0, d.length);
            return d;
        } catch (Exception e) {
            return new byte[0];
        } finally {
            close(rs);
        }
    }

    public static void write(String path, byte[] data, int len) throws IOException {
        RecordStore rs = open(path, true);
        if (rs == null) {
            throw new IOException("storage full");
        }
        try {
            ByteArrayOutputStream bo = new ByteArrayOutputStream(len + 64);
            DataOutputStream o = new DataOutputStream(bo);
            o.writeUTF(path);
            o.write(data, 0, len);
            byte[] r = bo.toByteArray();
            if (rs.getNumRecords() == 0) {
                rs.addRecord(r, 0, r.length);
            } else {
                rs.setRecord(1, r, 0, r.length);
            }
        } catch (IOException e) {
            throw e;
        } catch (Exception e) {
            throw new IOException(e.toString());
        } finally {
            close(rs);
        }
    }

    public static void delete(String path) {
        try {
            RecordStore.deleteRecordStore(storeName(path));
        } catch (Exception e) {
        }
    }

    /** Names of the files whose path starts with folder + "/". */
    public static String[] list(String folder) {
        String[] all = RecordStore.listRecordStores();
        java.util.Vector v = new java.util.Vector();
        String prefix = folder + "/";
        for (int i = 0; all != null && i < all.length; i++) {
            if (!all[i].startsWith("dfs")) {
                continue;
            }
            try {
                RecordStore rs = RecordStore.openRecordStore(all[i], false);
                try {
                    byte[] r = rs.getRecord(1);
                    java.io.DataInputStream in = new java.io.DataInputStream(
                            new java.io.ByteArrayInputStream(r));
                    String p = in.readUTF();
                    if (p.startsWith(prefix)) {
                        v.addElement(p.substring(prefix.length()));
                    }
                } finally {
                    close(rs);
                }
            } catch (Exception e) {
            }
        }
        String[] out = new String[v.size()];
        v.copyInto(out);
        return out;
    }
}
