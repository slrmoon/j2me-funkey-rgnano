/*
 * i-appli HTTP from recorded answers (see com.sun.midp.io.j2me.http.NetReplay):
 * Connector opens it for http:/https: when the launcher gave a .net file.
 * Without the file, or for a request that is not in it, the game gets the
 * IOException it got before (no network).
 */
package com.sun.midp.io.j2me.http;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import javax.microedition.io.Connection;

import com.nttdocomo.io.ConnectionException;
import com.nttdocomo.io.HttpConnection;
import com.sun.cldc.io.ConnectionBaseInterface;

public class ReplayHttp implements ConnectionBaseInterface, HttpConnection {
    private String url;
    private String method = GET;
    private ByteArrayOutputStream post;
    private NetReplay.Answer answer;
    private boolean asked;

    public ReplayHttp() {
    }

    public Connection openPrim(String name, int mode, boolean timeouts)
            throws IOException {
        if (!NetReplay.isActive()) {
            /* no .net file: as before, but every url in the log */
            NetReplay.find(GET, name, null);
            throw new javax.microedition.io.ConnectionNotFoundException(
                "no network for i-appli: " + name.substring(0, name.indexOf(':') + 1));
        }
        ReplayHttp c = new ReplayHttp();
        c.url = name;
        return c;
    }

    private NetReplay.Answer answer() throws IOException {
        if (!asked) {
            asked = true;
            answer = NetReplay.find(method, url, post == null ? null : post.toByteArray());
        }
        if (answer == null) {
            throw new ConnectionException(ConnectionException.NO_RESOURCES,
                                          "no network: " + url);
        }
        return answer;
    }

    public void connect() throws IOException {
        answer();
    }

    public void close() throws IOException {
    }

    public InputStream openInputStream() throws IOException {
        return new ByteArrayInputStream(answer().body);
    }

    public DataInputStream openDataInputStream() throws IOException {
        return new DataInputStream(openInputStream());
    }

    public OutputStream openOutputStream() throws IOException {
        if (post == null) {
            post = new ByteArrayOutputStream();
        }
        return post;
    }

    public DataOutputStream openDataOutputStream() throws IOException {
        return new DataOutputStream(openOutputStream());
    }

    public String getEncoding() {
        return answer == null ? null : answer.header("Content-Encoding");
    }

    public long getLength() {
        return answer == null ? -1 : answer.body.length;
    }

    public String getType() {
        return answer == null ? null : answer.header("Content-Type");
    }

    public String getURL() {
        return url;
    }

    public void setRequestMethod(String m) throws IOException {
        if (m != null) {
            method = m.toUpperCase();
        }
    }

    public void setRequestProperty(String key, String value) throws IOException {
    }

    public int getResponseCode() throws IOException {
        return answer().status;
    }

    public String getResponseMessage() throws IOException {
        return answer().message;
    }

    public String getHeaderField(String name) {
        return answer == null ? null : answer.header(name);
    }

    public long getDate() {
        return 0;
    }

    public long getExpiration() {
        return 0;
    }

    public long getLastModified() {
        return 0;
    }

    public void setIfModifiedSince(long timestamp) {
    }
}
