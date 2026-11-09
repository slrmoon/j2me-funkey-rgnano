package com.nttdocomo.device;

import java.io.IOException;

import com.nttdocomo.fs.AccessToken;
import com.nttdocomo.fs.Folder;

/** Every device name is a writable storage kept in the game's RMS. */
public class StorageDevice {
    public static final String CATEGORY_HARDWARE = "hardware";
    public static final String CAPABILITY_SD = "SD";
    public static final String CAPABILITY_MINISD = "miniSD";
    public static final String CATEGORY_FILESYSTEM = "filesystem";
    public static final String CAPABILITY_FAT12 = "FAT12";
    public static final String CAPABILITY_FAT16 = "FAT16";
    public static final String CAPABILITY_FAT32 = "FAT32";
    public static final String CAPABILITY_FAT_LONG_NAME = "FAT_LONG_NAME";
    public static final String CATEGORY_ENCRYPTION = "encryption";
    public static final String CAPABILITY_SD_BINDING = "SD-Binding";

    private String name;

    protected StorageDevice() {
    }

    public static StorageDevice getInstance(String deviceName) {
        if (deviceName == null) {
            throw new NullPointerException("deviceName");
        }
        StorageDevice d = new StorageDevice();
        d.name = deviceName;
        return d;
    }

    public String getDeviceName() {
        return name;
    }

    public String getPrintName() {
        return name;
    }

    public boolean isRemovable() {
        return true;
    }

    public boolean isAccessible() {
        return true;
    }

    public boolean isReadable() {
        return true;
    }

    public boolean isWritable() {
        return true;
    }

    public String[] getCapability(String category) throws IOException {
        if (CATEGORY_HARDWARE.equals(category)) {
            return new String[] {CAPABILITY_SD};
        }
        if (CATEGORY_FILESYSTEM.equals(category)) {
            return new String[] {CAPABILITY_FAT16, CAPABILITY_FAT_LONG_NAME};
        }
        return new String[0];
    }

    public String getMediaId() throws IOException {
        return "FUNKEY";
    }

    public Folder getFolder(AccessToken accessToken) throws IOException {
        if (accessToken == null) {
            throw new NullPointerException("accessToken");
        }
        return Folder.fkNew(this, accessToken);
    }
}
