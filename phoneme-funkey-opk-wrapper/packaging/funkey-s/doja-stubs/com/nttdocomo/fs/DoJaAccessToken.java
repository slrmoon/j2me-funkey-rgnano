package com.nttdocomo.fs;

public class DoJaAccessToken implements AccessToken {
    public static final int ACCESS_UIM = 0x01;
    public static final int ACCESS_PLATFORM = 0x02;
    public static final int ACCESS_SERIES = 0x04;

    int access, share;

    protected DoJaAccessToken() {
    }

    public int getAccess() {
        return access;
    }

    public int getShare() {
        return share;
    }

    String key() {
        return access + "|" + share;
    }
}
