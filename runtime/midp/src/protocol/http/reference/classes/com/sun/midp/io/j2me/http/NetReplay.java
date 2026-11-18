/*
 * Recorded server answers for games whose servers are gone or that
 * only fetch static files (the Keitai Archive revival servers
 * *.gs.keitaiarchive.org, http://localhost/... in archived packages).
 *
 * The launcher passes -Dnet.replay=<file> when a <game>.net file lies
 * next to the jar. HTTP requests are answered from it; there is no
 * network in this runtime otherwise (web, FunKey). A request that is
 * not in the file is logged as "network: <url>" (the catalog's tools
 * read it, fetch the answer and add it to the file).
 *
 * File format (bytes, ISO-8859-1 header lines):
 *   J2MENET 1\n
 *   then per answer:
 *   REQ <METHOD|*> <url>\n
 *   STATUS <code> <message>\n
 *   HDR <Name>: <value>\n          (any number)
 *   BODY <length>\n<length bytes>\n
 * An url recorded without "?query" also answers the same url with any
 * query. Dot segments ("/../") are resolved on both sides.
 */
package com.sun.midp.io.j2me.http;

import java.util.Vector;

import javax.microedition.io.Connector;

import com.sun.midp.io.j2me.storage.RandomAccessStream;

public final class NetReplay {

    /** one recorded answer */
    public static final class Answer {
        public String method;
        public String url;
        public int status = 200;
        public String message = "OK";
        public Vector headers = new Vector();   // String[2]
        public byte[] body = new byte[0];

        public String header(String name) {
            for (int i = 0; i < headers.size(); i++) {
                String[] h = (String[])headers.elementAt(i);
                if (h[0].equalsIgnoreCase(name)) {
                    return h[1];
                }
            }
            return null;
        }
    }

    private static Vector answers;
    private static boolean loaded;
    private static boolean active;
    private static int missesLogged;
    private static String missed = " ";

    private NetReplay() {
    }

    /** true when the launcher gave a replay file (even an empty one) */
    public static synchronized boolean isActive() {
        load();
        return active;
    }

    /** the recorded answer for this request, or null (logged) */
    public static synchronized Answer find(String method, String url, byte[] postBody) {
        load();
        String key = normalize(url);
        Answer best = null;
        for (int i = 0; answers != null && i < answers.size(); i++) {
            Answer a = (Answer)answers.elementAt(i);
            if (!a.method.equals("*") && !a.method.equalsIgnoreCase(method)) {
                continue;
            }
            String au = normalize(a.url);
            if (au.equals(key)) {
                best = a;
                break;
            }
            if (best == null && au.indexOf('?') < 0 && stripQuery(key).equals(au)) {
                best = a;
            }
        }
        if (best != null) {
            System.out.println("net-replay: " + method + " " + url + " -> "
                               + best.status + " (" + best.body.length + " bytes)");
            return best;
        }
        /* the classic line (runner: "needs network"), and the request in
         * full for the catalog's capture tool; every url once */
        String tag = method + " " + url;
        if (missed.indexOf(" " + tag + " ") < 0 && missesLogged < 200) {
            missesLogged++;
            missed += tag + " ";
            System.out.println("network: " + url);
            StringBuffer sb = new StringBuffer("net-miss: ").append(method)
                .append(' ').append(url);
            if (postBody != null && postBody.length > 0) {
                sb.append(" body=");
                int n = postBody.length < 4096 ? postBody.length : 4096;
                for (int i = 0; i < n; i++) {
                    int b = postBody[i] & 0xff;
                    sb.append(HEX.charAt(b >> 4)).append(HEX.charAt(b & 15));
                }
            }
            System.out.println(sb.toString());
        }
        return null;
    }

    private static final String HEX = "0123456789abcdef";

    private static String stripQuery(String u) {
        int q = u.indexOf('?');
        return q < 0 ? u : u.substring(0, q);
    }

    /** lower-case scheme and host, default port dropped, dot segments resolved */
    public static String normalize(String url) {
        if (url == null) {
            return "";
        }
        int ss = url.indexOf("://");
        if (ss < 0) {
            return url;
        }
        int pathStart = url.indexOf('/', ss + 3);
        String head = (pathStart < 0 ? url : url.substring(0, pathStart)).toLowerCase();
        if (head.startsWith("http://") && head.endsWith(":80")) {
            head = head.substring(0, head.length() - 3);
        }
        if (pathStart < 0) {
            return head + "/";
        }
        String rest = url.substring(pathStart);
        int q = rest.indexOf('?');
        String path = q < 0 ? rest : rest.substring(0, q);
        String query = q < 0 ? "" : rest.substring(q);
        Vector segs = new Vector();
        int i = 1;
        while (i <= path.length()) {
            int j = path.indexOf('/', i);
            if (j < 0) {
                j = path.length();
            }
            String s = path.substring(i, j);
            if (s.equals("..")) {
                if (segs.size() > 0) {
                    segs.removeElementAt(segs.size() - 1);
                }
            } else if (!s.equals(".")) {
                segs.addElement(s);
            }
            i = j + 1;
        }
        StringBuffer sb = new StringBuffer(head);
        for (int k = 0; k < segs.size(); k++) {
            sb.append('/').append((String)segs.elementAt(k));
        }
        if (segs.size() == 0 || path.endsWith("/")) {
            if (sb.charAt(sb.length() - 1) != '/') {
                sb.append('/');
            }
        }
        return sb.append(query).toString();
    }

    private static void load() {
        if (loaded) {
            return;
        }
        loaded = true;
        answers = new Vector();
        String path = System.getProperty("net.replay");
        if (path == null || path.length() == 0) {
            return;
        }
        RandomAccessStream s = new RandomAccessStream();
        try {
            s.connect(path, Connector.READ);
            int size = s.getSizeOf();
            byte[] b = new byte[size];
            int off = 0;
            while (off < size) {
                int n = s.readBytes(b, off, size - off);
                if (n <= 0) {
                    break;
                }
                off += n;
            }
            active = true;
            parse(b, off);
            System.out.println("net-replay: " + answers.size() + " answers from " + path);
        } catch (Throwable t) {
            System.out.println("net-replay: cannot read " + path + ": " + t);
        } finally {
            try {
                s.disconnect();
            } catch (Throwable t) {
            }
        }
    }

    private static void parse(byte[] b, int len) {
        int[] pos = {0};
        String magic = line(b, len, pos);
        if (magic == null || !magic.startsWith("J2MENET")) {
            System.out.println("net-replay: not a replay file");
            return;
        }
        Answer cur = null;
        String l;
        while ((l = line(b, len, pos)) != null) {
            if (l.startsWith("REQ ")) {
                int sp = l.indexOf(' ', 4);
                if (sp < 0) {
                    cur = null;
                    continue;
                }
                cur = new Answer();
                cur.method = l.substring(4, sp);
                cur.url = l.substring(sp + 1);
                answers.addElement(cur);
            } else if (cur == null) {
                continue;
            } else if (l.startsWith("STATUS ")) {
                String v = l.substring(7);
                int sp = v.indexOf(' ');
                try {
                    cur.status = Integer.parseInt(sp < 0 ? v : v.substring(0, sp));
                } catch (NumberFormatException e) {
                }
                cur.message = sp < 0 ? "" : v.substring(sp + 1);
            } else if (l.startsWith("HDR ")) {
                int c = l.indexOf(':', 4);
                if (c > 4) {
                    cur.headers.addElement(new String[] {
                        l.substring(4, c).trim(), l.substring(c + 1).trim()});
                }
            } else if (l.startsWith("BODY ")) {
                int n = 0;
                try {
                    n = Integer.parseInt(l.substring(5).trim());
                } catch (NumberFormatException e) {
                }
                if (n < 0 || pos[0] + n > len) {
                    n = Math.max(0, len - pos[0]);
                }
                cur.body = new byte[n];
                System.arraycopy(b, pos[0], cur.body, 0, n);
                pos[0] += n;
                if (pos[0] < len && b[pos[0]] == '\n') {
                    pos[0]++;
                }
            }
        }
    }

    private static String line(byte[] b, int len, int[] pos) {
        if (pos[0] >= len) {
            return null;
        }
        int start = pos[0];
        int i = start;
        while (i < len && b[i] != '\n') {
            i++;
        }
        pos[0] = i < len ? i + 1 : len;
        int end = i;
        if (end > start && b[end - 1] == '\r') {
            end--;
        }
        char[] c = new char[end - start];
        for (int k = 0; k < c.length; k++) {
            c[k] = (char)(b[start + k] & 0xff);
        }
        return new String(c);
    }
}
