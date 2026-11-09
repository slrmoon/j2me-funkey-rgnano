package com.nttdocomo.ui;

/** Stand-by application ("i-appli stand-by"); runs like a normal application. */
public abstract class MApplication extends IApplication {
    public static final int MODE_CHANGED_EVENT = 1;
    public static final int WAKEUP_TIMER_EVENT = 2;
    public static final int CLOCK_TICK_EVENT = 3;
    public static final int FOLD_CHANGED_EVENT = 4;

    private int wakeup = -1;

    public MApplication() {
    }

    public void processSystemEvent(int type, int param) {
    }

    public final void sleep() {
    }

    public final void setWakeupTimer(int minutes) {
        wakeup = minutes;
    }

    public final int getWakeupTimer() {
        return wakeup;
    }

    public final void resetWakeupTimer() {
        wakeup = -1;
    }

    public final void setClockTick(boolean enabled) {
    }

    public final void deactivate() {
    }

    public final boolean isActive() {
        return true;
    }
}
