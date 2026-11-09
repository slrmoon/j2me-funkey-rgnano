package com.nttdocomo.util;

import com.funkey.doja.DoJaRuntime;

/**
 * General timer; the listener runs on the DoJa event thread.
 */
public final class Timer implements TimeKeeper {
    private int time;
    private boolean repeat;
    private TimerListener listener;
    private java.util.Timer timer;
    private volatile boolean pending;
    private boolean disposed;

    public Timer() {
    }

    public int getMinTimeInterval() {
        return 10;
    }

    public int getResolution() {
        return 10;
    }

    public void setRepeat(boolean r) {
        repeat = r;
    }

    public void setTime(int t) {
        if (t < 0) {
            throw new IllegalArgumentException("time");
        }
        time = t;
    }

    public void setListener(TimerListener l) {
        listener = l;
    }

    public synchronized void start() {
        if (disposed) {
            throw new com.nttdocomo.lang.IllegalStateException("disposed");
        }
        if (timer != null) {
            return;
        }
        int t = time <= 0 ? 1 : time;
        timer = new java.util.Timer();
        java.util.TimerTask task = new java.util.TimerTask() {
            public void run() {
                fire();
            }
        };
        if (repeat) {
            timer.schedule(task, t, t);
        } else {
            timer.schedule(task, t);
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
                TimerListener l = listener;
                if (l != null) {
                    l.timerExpired(Timer.this);
                }
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
        listener = null;
    }
}
