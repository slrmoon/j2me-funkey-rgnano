package com.nttdocomo.ui;

import com.funkey.doja.DoJaRuntime;
import com.nttdocomo.util.TimeKeeper;

/**
 * Timer that delivers Display.TIMER_EXPIRED_EVENT to a canvas.
 */
public final class ShortTimer implements TimeKeeper {
    private Canvas canvas;
    private int id;
    private int interval;
    private boolean repeat;
    private java.util.Timer timer;
    private volatile boolean pending;
    private boolean disposed;

    protected ShortTimer() {
    }

    public static ShortTimer getShortTimer(Canvas canvas, int timerId, int interval, boolean repeat) {
        if (canvas == null) {
            throw new NullPointerException("canvas");
        }
        if (interval <= 0) {
            throw new IllegalArgumentException("interval");
        }
        ShortTimer t = new ShortTimer();
        t.canvas = canvas;
        t.id = timerId;
        t.interval = interval;
        t.repeat = repeat;
        return t;
    }

    public int getMinTimeInterval() {
        return 10;
    }

    public int getResolution() {
        return 10;
    }

    public synchronized void start() {
        if (disposed) {
            throw new UIException(UIException.ILLEGAL_STATE, "disposed");
        }
        if (timer != null) {
            return;
        }
        timer = new java.util.Timer();
        java.util.TimerTask task = new java.util.TimerTask() {
            public void run() {
                fire();
            }
        };
        if (repeat) {
            timer.schedule(task, interval, interval);
        } else {
            timer.schedule(task, interval);
        }
    }

    private void fire() {
        if (!repeat) {
            synchronized (this) {
                if (timer != null) {
                    timer.cancel();
                    timer = null;
                }
            }
        }
        if (pending) {
            return;
        }
        pending = true;
        DoJaRuntime.postRun(new Runnable() {
            public void run() {
                pending = false;
                canvas.processEvent(Display.TIMER_EXPIRED_EVENT, id);
            }
        });
    }

    public synchronized void stop() {
        if (timer != null) {
            timer.cancel();
            timer = null;
        }
    }

    public void dispose() {
        stop();
        disposed = true;
    }
}
