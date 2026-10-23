package com.nttdocomo.fs;

import java.io.IOException;

import com.funkey.doja.FsStore;
import com.nttdocomo.device.StorageDevice;

public class Folder {
    private StorageDevice device;
    private AccessToken token;
    private String path;
    String key;

    protected Folder(StorageDevice storageDevice, AccessToken accessToken, String path) {
        device = storageDevice;
        token = accessToken;
        this.path = path;
        String t = accessToken instanceof DoJaAccessToken ? ((DoJaAccessToken) accessToken).key() : "0|0";
        key = (storageDevice == null ? "" : storageDevice.getDeviceName()) + "|" + t + path;
    }

    protected Folder() {
    }

    /** runtime: StorageDevice.getFolder */
    public static Folder fkNew(StorageDevice d, AccessToken t) {
        return new Folder(d, t, "/");
    }

    public StorageDevice getStorageDevice() {
        return device;
    }

    public AccessToken getAccessToken() {
        return token;
    }

    public String getPath() {
        return path;
    }

    public long getFreeSize() throws IOException {
        return 4L * 1024 * 1024;
    }

    public boolean isFileAttributeSupported(Class clazz) {
        return false;
    }

    private static void checkName(String name) throws IOException {
        if (name == null) {
            throw new NullPointerException("fileName");
        }
        if (name.length() == 0 || name.indexOf('/') >= 0 || name.indexOf('\\') >= 0) {
            throw new FileNotAccessibleException(FileNotAccessibleException.ILLEGAL_NAME, name);
        }
    }

    public File createFile(String fileName) throws IOException {
        return createFile(fileName, null);
    }

    public File createFile(String fileName, FileAttribute[] attributes) throws IOException {
        checkName(fileName);
        String p = key + "/" + fileName;
        if (FsStore.exists(p)) {
            throw new FileNotAccessibleException(FileNotAccessibleException.ALREADY_EXISTS, fileName);
        }
        FsStore.write(p, new byte[0], 0);
        return new File(this, fileName);
    }

    public File[] getFiles() throws IOException {
        String[] names = FsStore.list(key);
        File[] f = new File[names.length];
        for (int i = 0; i < names.length; i++) {
            f[i] = new File(this, names[i]);
        }
        return f;
    }

    public File getFile(String fileName) throws IOException {
        checkName(fileName);
        if (!FsStore.exists(key + "/" + fileName)) {
            throw new FileNotAccessibleException(FileNotAccessibleException.NOT_FOUND, fileName);
        }
        return new File(this, fileName);
    }
}
