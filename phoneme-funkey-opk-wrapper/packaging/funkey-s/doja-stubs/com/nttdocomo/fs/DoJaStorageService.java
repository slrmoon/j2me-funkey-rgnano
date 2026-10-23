package com.nttdocomo.fs;

public class DoJaStorageService {
    public static final int SHARE_APPLICATION = 0x08;
    public static final int SHARE_CONTENTS_PROVIDER = 0x10;

    protected DoJaStorageService() {
    }

    public static DoJaAccessToken getAccessToken(int access, int share) {
        DoJaAccessToken t = new DoJaAccessToken();
        t.access = access;
        t.share = share;
        return t;
    }
}
