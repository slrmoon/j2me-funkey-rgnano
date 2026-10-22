package com.funkey.doja;

import java.io.IOException;
import java.util.Hashtable;
import java.util.Vector;

import javax.microedition.lcdui.Display;
import javax.microedition.midlet.MIDlet;

import com.nttdocomo.ui.Frame;
import com.nttdocomo.ui.FkBridge;
import com.nttdocomo.ui.IApplication;

/**
 * Process-wide state of the DoJa runtime: the .jam properties, the host
 * canvas, keypad state and the single DoJa event thread.
 *
 * DoJa delivers every callback (key events, timers, media events, paint)
 * on one thread. MIDP key callbacks only update the keypad state and queue
 * an event here.
 */
public final class DoJaRuntime implements Runnable {
    private static MIDlet midlet;
    private static DoJaHost host;
    private static Hashtable jam = new Hashtable();
    private static int drawWidth;
    private static int drawHeight;
    private static int profileMajor = -1;
    private static int profileMinor;

    private static volatile long keypadState;
    private static IApplication app;
    private static Thread eventThread;
    private static DoJaRuntime loop;
    private static final Vector queue = new Vector();
    private static volatile boolean terminated;

    /* event kinds */
    public static final int EV_KEY = 1;
    public static final int EV_PAINT = 2;
    public static final int EV_RUN = 3;
    public static final int EV_RESUME = 4;

    private DoJaRuntime() {
    }

    /* ------------------------------------------------------------------ */
    /* startup                                                             */

    public static void init(MIDlet m) {
        midlet = m;
        loadJam();
        host = new DoJaHost();
        Display.getDisplay(m).setCurrent(host);
        computeDrawArea();
    }

    public static void startApplication(final String className) {
        loop = new DoJaRuntime();
        eventThread = new Thread(loop);
        postRun(new Runnable() {
            public void run() {
                try {
                    Object o = Class.forName(className).newInstance();
                    app = (IApplication) o;
                    IApplication.setCurrentApp(app);
                    app.start();
                } catch (Throwable t) {
                    System.out.println("DoJa: application start failed: " + t);
                    t.printStackTrace();
                }
            }
        });
        eventThread.start();
    }

    public static MIDlet getMIDlet() {
        return midlet;
    }

    public static Display getMIDPDisplay() {
        return Display.getDisplay(midlet);
    }

    public static DoJaHost host() {
        return host;
    }

    public static IApplication application() {
        return app;
    }

    /* ------------------------------------------------------------------ */
    /* .jam properties                                                     */

    private static void loadJam() {
        String hex = System.getProperty("doja.jam.hex");
        if (hex != null && hex.length() >= 2) {
            byte[] raw = new byte[hex.length() / 2];
            for (int i = 0; i < raw.length; i++) {
                raw[i] = (byte) ((hexDigit(hex.charAt(2 * i)) << 4)
                        | hexDigit(hex.charAt(2 * i + 1)));
            }
            String text;
            try {
                text = new String(raw, "SJIS");
            } catch (Throwable t) {
                text = new String(raw);
            }
            parseJam(text);
        }
        putIfMissing("AppClass", sysprop("doja.app.class"));
        putIfMissing("AppParam", sysprop("doja.app.param"));
        putIfMissing("AppVer", sysprop("doja.app.ver"));
        putIfMissing("DrawArea", sysprop("doja.draw.area"));
        putIfMissing("ProfileVer", sysprop("doja.profile.ver"));
        String pv = getJam("ProfileVer");
        if (pv != null) {
            int i = pv.indexOf('-');
            String v = i >= 0 ? pv.substring(i + 1) : pv;
            int dot = v.indexOf('.');
            try {
                if (dot > 0) {
                    profileMajor = Integer.parseInt(v.substring(0, dot).trim());
                    String minor = v.substring(dot + 1).trim();
                    int end = 0;
                    while (end < minor.length() && Character.isDigit(minor.charAt(end))) {
                        end++;
                    }
                    profileMinor = end > 0 ? Integer.parseInt(minor.substring(0, end)) : 0;
                } else {
                    profileMajor = Integer.parseInt(v.trim());
                }
            } catch (Throwable t) {
                profileMajor = -1;
            }
        }
    }

    private static String sysprop(String key) {
        try {
            return System.getProperty(key);
        } catch (Throwable t) {
            return null;
        }
    }

    private static int hexDigit(char c) {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return 0;
    }

    private static void putIfMissing(String key, String value) {
        if (value != null && value.length() > 0 && jam.get(key) == null) {
            jam.put(key, value);
        }
    }

    private static void parseJam(String text) {
        int pos = 0;
        int n = text.length();
        while (pos < n) {
            int end = pos;
            while (end < n && text.charAt(end) != '\n' && text.charAt(end) != '\r') {
                end++;
            }
            String line = text.substring(pos, end);
            pos = end + 1;
            int eq = line.indexOf('=');
            if (eq <= 0) {
                continue;
            }
            String key = line.substring(0, eq).trim();
            String value = line.substring(eq + 1).trim();
            if (key.length() > 0 && jam.get(key) == null) {
                jam.put(key, value);
            }
        }
    }

    public static String getJam(String key) {
        return (String) jam.get(key);
    }

    /** DoJa profile major version from ProfileVer, or -1 when unknown. */
    public static int profileMajor() {
        return profileMajor;
    }

    public static int profileMinor() {
        return profileMinor;
    }

    /* ------------------------------------------------------------------ */
    /* screen                                                              */

    private static void computeDrawArea() {
        int w = 0;
        int h = 0;
        String da = getJam("DrawArea");
        if (da != null) {
            int x = da.indexOf('x');
            if (x < 0) {
                x = da.indexOf('X');
            }
            if (x > 0) {
                try {
                    w = Integer.parseInt(da.substring(0, x).trim());
                    h = Integer.parseInt(da.substring(x + 1).trim());
                } catch (Throwable t) {
                    w = h = 0;
                }
            }
        }
        if (w <= 0 || h <= 0) {
            w = host.getWidth();
            h = host.getHeight();
            if (w <= 0 || h <= 0) {
                w = 240;
                h = 240;
            }
        }
        drawWidth = w;
        drawHeight = h;
        System.out.println("DoJa: draw area " + w + "x" + h + " (host "
                + host.getWidth() + "x" + host.getHeight() + ") profile "
                + getJam("ProfileVer"));
    }

    public static int drawWidth() {
        return drawWidth;
    }

    public static int drawHeight() {
        return drawHeight;
    }

    /** Shows the current frame: repaint the host canvas and wait for it. */
    public static void present() {
        if (TRACE) {
            System.out.println("DoJa trace: present " + Thread.currentThread());
        }
        DoJaHost h = host;
        if (h != null) {
            h.repaint();
            h.serviceRepaints();
        }
    }

    private static final Object autoLock = new Object();
    private static boolean autoRequested;
    private static Thread autoThread;

    /**
     * Drawing outside lock()/unlock() shows up on a handset right away; here
     * it is presented shortly after, batching consecutive draw calls.
     */
    public static void requestAutoPresent() {
        synchronized (autoLock) {
            if (autoRequested) {
                return;
            }
            autoRequested = true;
            if (autoThread == null) {
                autoThread = new Thread() {
                    public void run() {
                        autoPresentLoop();
                    }
                };
                autoThread.start();
            }
            autoLock.notify();
        }
    }

    private static void autoPresentLoop() {
        while (!terminated) {
            synchronized (autoLock) {
                while (!autoRequested && !terminated) {
                    try {
                        autoLock.wait();
                    } catch (InterruptedException ie) {
                        return;
                    }
                }
            }
            try {
                Thread.sleep(16);
            } catch (InterruptedException ie) {
                return;
            }
            synchronized (autoLock) {
                autoRequested = false;
            }
            present();
        }
    }

    public static void frameChanged() {
        DoJaHost h = host;
        if (h != null) {
            h.repaint();
        }
    }

    /* ------------------------------------------------------------------ */
    /* keys                                                                */

    public static int keypadState() {
        return (int) keypadState;
    }

    /** group 0: keys 0..31, group 1: keys 32..63 */
    public static int keypadState(int group) {
        return group == 0 ? (int) keypadState : group == 1 ? (int) (keypadState >>> 32) : 0;
    }

    static void keyEvent(int dojaKey, boolean pressed) {
        if (dojaKey < 0 || dojaKey > 63) {
            return;
        }
        long bit = 1L << dojaKey;
        if (pressed) {
            if ((keypadState & bit) != 0) {
                return;
            }
            keypadState |= bit;
        } else {
            if ((keypadState & bit) == 0) {
                return;
            }
            keypadState &= ~bit;
        }
        post(new Event(EV_KEY, pressed ? 0 : 1, dojaKey, null));
    }

    public static void releaseKeysForModal() {
        releaseAllKeys();
    }

    static void releaseAllKeys() {
        long state = keypadState;
        for (int k = 0; k < 64; k++) {
            if ((state & (1L << k)) != 0) {
                keyEvent(k, false);
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /* event queue                                                         */

    static final class Event {
        final int kind;
        final int a;
        final int b;
        final Object o;

        Event(int kind, int a, int b, Object o) {
            this.kind = kind;
            this.a = a;
            this.b = b;
            this.o = o;
        }
    }

    public static void postRun(Runnable r) {
        post(new Event(EV_RUN, 0, 0, r));
    }

    public static void postRepaint(Frame f) {
        post(new Event(EV_PAINT, 0, 0, f));
    }

    public static void postResume() {
        post(new Event(EV_RESUME, 0, 0, null));
    }

    private static void post(Event e) {
        synchronized (queue) {
            queue.addElement(e);
            queue.notify();
        }
    }

    public static boolean isEventThread() {
        return Thread.currentThread() == eventThread;
    }

    /**
     * Runs queued events until the condition object says stop; used by
     * modal dialogs shown from the event thread.
     */
    public static void pumpUntil(ModalCondition cond) {
        while (!cond.done() && !terminated) {
            Event e = take(50);
            if (e != null) {
                dispatch(e);
            }
        }
    }

    public interface ModalCondition {
        boolean done();
    }

    private static Event take(long timeout) {
        synchronized (queue) {
            if (queue.size() == 0) {
                try {
                    if (timeout > 0) {
                        queue.wait(timeout);
                    } else {
                        queue.wait();
                    }
                } catch (InterruptedException ie) {
                    return null;
                }
            }
            if (queue.size() == 0) {
                return null;
            }
            Event e = (Event) queue.elementAt(0);
            queue.removeElementAt(0);
            return e;
        }
    }

    public void run() {
        while (!terminated) {
            Event e = take(0);
            if (e != null) {
                dispatch(e);
            }
        }
    }

    static final boolean TRACE = "1".equals(sysprop("doja.trace"));

    private static void dispatch(Event e) {
        if (TRACE) {
            System.out.println("DoJa trace: event " + e.kind + " " + e.a + " " + e.b + " " + e.o);
        }
        try {
            switch (e.kind) {
            case EV_RUN:
                ((Runnable) e.o).run();
                break;
            case EV_KEY: {
                Frame f = com.nttdocomo.ui.Display.getCurrent();
                if (f != null) {
                    FkBridge.dispatchKey(f, e.a, e.b);
                }
                break;
            }
            case EV_PAINT:
                FkBridge.dispatchPaint((Frame) e.o);
                break;
            case EV_RESUME:
                if (app != null) {
                    app.resume();
                }
                break;
            default:
                break;
            }
        } catch (Throwable t) {
            System.out.println("DoJa: uncaught exception in event handler: " + t);
            t.printStackTrace();
        }
    }

    /* ------------------------------------------------------------------ */
    /* shutdown                                                            */

    public static void terminate() {
        terminated = true;
        synchronized (queue) {
            queue.notify();
        }
        com.nttdocomo.ui.AudioPresenter.stopAll();
        MIDlet m = midlet;
        if (m != null) {
            m.notifyDestroyed();
        }
    }

    public static boolean isTerminated() {
        return terminated;
    }

    /* ------------------------------------------------------------------ */
    /* helpers kept for older callers                                      */

    public static String getenv(String key) {
        if ("DOJA_APP_CLASS".equals(key)) {
            return getJam("AppClass");
        }
        if ("DOJA_APP_PARAM".equals(key)) {
            return getJam("AppParam");
        }
        if ("DOJA_APP_VER".equals(key)) {
            return getJam("AppVer");
        }
        if ("DOJA_SOURCE_URL".equals(key)) {
            String u = getJam("PackageURL");
            return u != null ? u : sysprop("doja.source.url");
        }
        return sysprop(key);
    }

    public static byte[] readResource(String name) throws IOException {
        return new ResourceLoader().read(name);
    }
}
