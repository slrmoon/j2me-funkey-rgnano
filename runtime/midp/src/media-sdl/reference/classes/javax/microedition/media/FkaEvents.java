/*
 * FunKey audio engine: one thread for every player.
 *
 * It picks up native events (end of media, stop time) and delivers
 * PlayerListener callbacks in order. While something plays it polls every
 * 20 ms with sleep; when nothing plays it waits, so an idle game costs no
 * CPU (the old players spun on Thread.yield()).
 */
package javax.microedition.media;

import java.util.Hashtable;
import java.util.Vector;

final class FkaEvents implements Runnable {
    private static final int POLL_MS = 20;

    private static final Object lock = new Object();
    private static final Object pumpLock = new Object();
    private static final Hashtable players = new Hashtable();
    private static final Vector queue = new Vector();
    private static Thread thread;
    private static int playing;

    private FkaEvents() {
    }

    static void register(FkaPlayer p) {
        players.put(new Integer(p.nativeId()), p);
    }

    static void unregister(FkaPlayer p) {
        players.remove(new Integer(p.nativeId()));
    }

    static void playerStarted() {
        synchronized (lock) {
            playing++;
            ensureThread();
            lock.notify();
        }
    }

    static void playerStopped() {
        synchronized (lock) {
            if (playing > 0) playing--;
        }
    }

    static void post(FkaPlayer p, String event, Object data) {
        synchronized (lock) {
            queue.addElement(new Object[] {p, event, data});
            ensureThread();
            lock.notify();
        }
    }

    private static void ensureThread() {
        if (thread == null) {
            thread = new Thread(new FkaEvents());
            thread.start();
        }
    }

    /* Moves native events into player state (and the listener queue). */
    static void pump() {
        synchronized (pumpLock) {
            long[] ev = new long[4];
            while (FkaNative.nPollEvent(ev)) {
                FkaPlayer p = (FkaPlayer) players.get(new Integer((int) ev[0]));
                if (p != null) p.nativeEvent((int) ev[1], ev[2], (int) ev[3]);
            }
        }
    }

    public void run() {
        for (;;) {
            Object[] item = null;
            try {
                pump();
                synchronized (lock) {
                    if (queue.isEmpty()) {
                        if (playing > 0) lock.wait(POLL_MS);
                        else lock.wait(1000);
                    }
                    if (!queue.isEmpty()) {
                        item = (Object[]) queue.elementAt(0);
                        queue.removeElementAt(0);
                    }
                }
                if (item != null) ((FkaPlayer) item[0]).deliver((String) item[1], item[2]);
            } catch (Throwable t) {
                /* keep the event thread alive whatever a listener does */
            }
        }
    }
}
