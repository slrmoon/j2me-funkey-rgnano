/*
 * MIDP HttpConnection answered from the recorded file (NetReplay). Connector
 * opens it for http:/https: when the launcher gave a .net file.
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
import javax.microedition.io.HttpConnection;

import com.sun.cldc.io.ConnectionBaseInterface;

public class ReplayConnection implements ConnectionBaseInterface, HttpConnection {
    private String url;
    private String method = GET;
    private ByteArrayOutputStream post;
    private NetReplay.Answer answer;
    private boolean asked;
    private java.util.Hashtable props = new java.util.Hashtable();

    public ReplayConnection() {
    }

    public Connection openPrim(String name, int mode, boolean timeouts)
            throws IOException {
        if (!NetReplay.isActive()) {
            throw new javax.microedition.io.ConnectionNotFoundException("no replay file");
        }
        ReplayConnection c = new ReplayConnection();
        c.url = name;
        return c;
    }

    private NetReplay.Answer answer() throws IOException {
        if (!asked) {
            asked = true;
            answer = NetReplay.find(method, url, post == null ? null : post.toByteArray());
        }
        if (answer == null) {
            throw new IOException("no network: " + url);
        }
        return answer;
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

    public String getType() {
        return getHeaderField("Content-Type");
    }

    public String getEncoding() {
        return getHeaderField("Content-Encoding");
    }

    public long getLength() {
        try {
            return answer().body.length;
        } catch (IOException e) {
            return -1;
        }
    }

    public String getURL() {
        return url;
    }

    public String getProtocol() {
        int c = url.indexOf(':');
        return c < 0 ? "http" : url.substring(0, c);
    }

    private String afterHost() {
        int ss = url.indexOf("//");
        int p = ss < 0 ? -1 : url.indexOf('/', ss + 2);
        return p < 0 ? "" : url.substring(p);
    }

    private String hostPort() {
        int ss = url.indexOf("//");
        if (ss < 0) {
            return "";
        }
        int p = url.indexOf('/', ss + 2);
        return p < 0 ? url.substring(ss + 2) : url.substring(ss + 2, p);
    }

    public String getHost() {
        String hp = hostPort();
        int c = hp.indexOf(':');
        return c < 0 ? hp : hp.substring(0, c);
    }

    public int getPort() {
        String hp = hostPort();
        int c = hp.indexOf(':');
        if (c >= 0) {
            try {
                return Integer.parseInt(hp.substring(c + 1));
            } catch (NumberFormatException e) {
            }
        }
        return getProtocol().equals("https") ? 443 : 80;
    }

    public String getFile() {
        String r = afterHost();
        int q = r.indexOf('?');
        int h = r.indexOf('#');
        int e = q >= 0 ? q : (h >= 0 ? h : r.length());
        return e == 0 ? null : r.substring(0, e);
    }

    public String getQuery() {
        String r = afterHost();
        int q = r.indexOf('?');
        if (q < 0) {
            return null;
        }
        int h = r.indexOf('#', q);
        return h < 0 ? r.substring(q + 1) : r.substring(q + 1, h);
    }

    public String getRef() {
        int h = url.indexOf('#');
        return h < 0 ? null : url.substring(h + 1);
    }

    public String getRequestMethod() {
        return method;
    }

    public void setRequestMethod(String m) throws IOException {
        if (m != null) {
            method = m.toUpperCase();
        }
    }

    public String getRequestProperty(String key) {
        return (String)props.get(key);
    }

    public void setRequestProperty(String key, String value) throws IOException {
        if (key != null && value != null) {
            props.put(key, value);
        }
    }

    public int getResponseCode() throws IOException {
        return answer().status;
    }

    public String getResponseMessage() throws IOException {
        return answer().message;
    }

    public long getExpiration() throws IOException {
        return getHeaderFieldDate("expires", 0);
    }

    public long getDate() throws IOException {
        return getHeaderFieldDate("date", 0);
    }

    public long getLastModified() throws IOException {
        return getHeaderFieldDate("last-modified", 0);
    }

    public String getHeaderField(String name) {
        try {
            return answer().header(name);
        } catch (IOException e) {
            return null;
        }
    }

    public int getHeaderFieldInt(String name, int def) throws IOException {
        String v = getHeaderField(name);
        try {
            return v == null ? def : Integer.parseInt(v.trim());
        } catch (NumberFormatException e) {
            return def;
        }
    }

    public long getHeaderFieldDate(String name, long def) throws IOException {
        return def;
    }

    public String getHeaderField(int n) throws IOException {
        java.util.Vector h = answer().headers;
        return n < 0 || n >= h.size() ? null : ((String[])h.elementAt(n))[1];
    }

    public String getHeaderFieldKey(int n) throws IOException {
        java.util.Vector h = answer().headers;
        return n < 0 || n >= h.size() ? null : ((String[])h.elementAt(n))[0];
    }
}
