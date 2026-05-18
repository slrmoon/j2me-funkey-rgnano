package com.nttdocomo.ui;

import java.util.Timer;
import java.util.TimerTask;

public abstract class ShortTimer {
    private Timer timer;
    private int interval;
    private boolean repeat;

    public abstract void timerExpired();

    public static ShortTimer getShortTimer(final Canvas canvas, final int param,
            int interval, boolean repeat) {
        ShortTimer timer = new ShortTimer() {
            public void timerExpired() {
                if (canvas != null) {
                    canvas.processEvent(Display.TIMER_EXPIRED_EVENT, param);
                }
            }
        };
        timer.interval = interval;
        timer.repeat = repeat;
        return timer;
    }

    public void start() {
        start(interval);
    }

    public void start(int interval) {
        stop();
        this.interval = interval;
        timer = new Timer();
        TimerTask task = new TimerTask() {
            public void run() {
                timerExpired();
            }
        };
        if (repeat) {
            timer.schedule(task, interval, interval);
        } else {
            timer.schedule(task, interval);
        }
    }

    public void stop() {
        if (timer != null) {
            timer.cancel();
            timer = null;
        }
    }

    public void dispose() {
        stop();
    }
}
