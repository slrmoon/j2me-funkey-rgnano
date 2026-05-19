package com.nttdocomo.system;

// UNSUPPORTED: This API is device-dependent and not fully implemented
import com.nttdocomo.lang.UnsupportedOperationException;

public class MessageAgent {

    public void delete(int p0, int p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public int[] getIds(int p0, boolean p1) {
    return null;
    }

    public com.nttdocomo.system.Message getMessage(int p0, int p1) {
    return null;
    }

    public boolean isSeen(int p0) {
    return false;
    }

    public boolean send(com.nttdocomo.system.MessageDraft p0) {
    return false;
    }

    public boolean send(com.nttdocomo.system.MessageSent p0) {
    return false;
    }

    public boolean send(java.lang.String p0, com.nttdocomo.lang.XString p1, java.lang.String p2, byte[] p3) {
    return false;
    }

    public boolean send(java.lang.String p0, java.lang.String[] p1, java.lang.String p2, byte[] p3) {
    return false;
    }

    public void setMessageFolderListener(com.nttdocomo.system.MessageFolderListener p0) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public void setSeen(int p0, boolean p1) {
    throw new com.nttdocomo.lang.UnsupportedOperationException();
    }

    public int size(int p0, boolean p1) {
    return 0;
    }

}