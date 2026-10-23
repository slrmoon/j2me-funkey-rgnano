package com.nttdocomo.fs;

import java.io.IOException;

import com.funkey.doja.FsStore;
import com.nttdocomo.io.FileEntity;

public class File {
    public static final int MODE_READ_ONLY = 0;
    public static final int MODE_WRITE_ONLY = 1;
    public static final int MODE_READ_WRITE = 2;

    private Folder folder;
    private String name;

    protected File() {
    }

    File(Folder folder, String name) {
        this.folder = folder;
        this.name = name;
    }

    private String store() {
        return folder.key + "/" + name;
    }

    public Folder getFolder() {
        return folder;
    }

    public AccessToken getAccessToken() {
        return folder.getAccessToken();
    }

    public String getPath() {
        return folder.getPath() + name;
    }

    public FileEntity open(int mode) throws IOException {
        if (mode != MODE_READ_ONLY && mode != MODE_WRITE_ONLY && mode != MODE_READ_WRITE) {
            throw new IllegalArgumentException("mode");
        }
        if (!FsStore.exists(store())) {
            throw new FileNotAccessibleException(FileNotAccessibleException.NOT_FOUND, name);
        }
        return FileEntity.fkOpen(store(), mode != MODE_WRITE_ONLY, mode != MODE_READ_ONLY);
    }

    public void delete() throws IOException {
        if (!FsStore.exists(store())) {
            throw new FileNotAccessibleException(FileNotAccessibleException.NOT_FOUND, name);
        }
        FsStore.delete(store());
    }

    public long getLength() throws IOException {
        byte[] d = FsStore.read(store());
        if (d == null) {
            throw new FileNotAccessibleException(FileNotAccessibleException.NOT_FOUND, name);
        }
        return d.length;
    }

    public long getLastModified() throws IOException {
        return System.currentTimeMillis();
    }
}
