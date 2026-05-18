package com.nttdocomo.io;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public interface HttpConnection extends javax.microedition.io.HttpConnection {
    void close() throws IOException;

    void connect() throws IOException;

    String getHeaderField(String name) throws IOException;

    long getLength();

    InputStream openInputStream() throws IOException;

    OutputStream openOutputStream() throws IOException;

    void setRequestMethod(String method) throws IOException;
}
