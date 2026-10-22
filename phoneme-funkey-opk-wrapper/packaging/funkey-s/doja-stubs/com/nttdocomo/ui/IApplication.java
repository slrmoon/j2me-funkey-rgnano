package com.nttdocomo.ui;

import java.util.Vector;

import com.funkey.doja.DoJaRuntime;
import com.nttdocomo.util.ScheduleDate;

public abstract class IApplication {
    public static final int LAUNCHED_FROM_MENU = 0;
    public static final int LAUNCHED_AFTER_DOWNLOAD = 1;
    public static final int LAUNCHED_FROM_TIMER = 2;
    public static final int LAUNCHED_AS_CONCIERGE = 3;
    public static final int LAUNCHED_FROM_EXT = 4;
    public static final int LAUNCHED_FROM_BROWSER = 5;
    public static final int LAUNCHED_FROM_MAILER = 6;
    public static final int LAUNCHED_FROM_IAPPLI = 7;
    public static final int LAUNCHED_FROM_LAUNCHER = 8;
    public static final int LAUNCHED_AS_ILET = 9;
    public static final int LAUNCHED_MSG_RECEIVED = 10;
    public static final int LAUNCHED_MSG_SENT = 11;
    public static final int LAUNCHED_MSG_UNSENT = 12;
    public static final int LAUNCHED_FROM_LOCATION_INFO = 13;
    public static final int LAUNCHED_FROM_LOCATION_IMAGE = 14;
    public static final int LAUNCHED_FROM_PHONEBOOK = 15;
    public static final int LAUNCHED_FROM_DTV = 17;
    public static final int LAUNCHED_FROM_TORUCA = 18;
    public static final int LAUNCHED_FROM_FELICA_ADHOC = 19;
    public static final int LAUNCHED_FROM_MENU_FOR_DELETION = 20;
    public static final int LAUNCHED_FROM_BML = 21;
    public static final int LAUNCH_BROWSER = 1;
    public static final int LAUNCH_VERSIONUP = 2;
    public static final int LAUNCH_IAPPLI = 3;
    public static final int LAUNCH_AS_LAUNCHER = 4;
    public static final int LAUNCH_MAILMENU = 5;
    public static final int LAUNCH_SCHEDULER = 6;
    public static final int LAUNCH_MAIL_RECEIVED = 7;
    public static final int LAUNCH_MAIL_SENT = 8;
    public static final int LAUNCH_MAIL_UNSENT = 9;
    public static final int LAUNCH_MAIL_LAST_INCOMING = 10;
    public static final int LAUNCH_DTV = 12;
    public static final int LAUNCH_BROWSER_SUSPEND = 13;
    public static final int SUSPEND_BY_NATIVE = 1;
    public static final int SUSPEND_BY_IAPP = 2;
    public static final int SUSPEND_PACKETIN = 256;
    public static final int SUSPEND_CALL_OUT = 512;
    public static final int SUSPEND_CALL_IN = 1024;
    public static final int SUSPEND_MAIL_SEND = 2048;
    public static final int SUSPEND_MAIL_RECEIVE = 4096;
    public static final int SUSPEND_MESSAGE_RECEIVE = 8192;
    public static final int SUSPEND_SCHEDULE_NOTIFY = 16384;
    public static final int SUSPEND_MULTITASK_APPLICATION = 32768;

    private static IApplication currentApp;
    private String[] args;

    public IApplication() {
        if (currentApp == null) {
            currentApp = this;
        }
    }

    public static IApplication getCurrentApp() {
        return currentApp;
    }

    /** runtime use */
    public static void setCurrentApp(IApplication app) {
        currentApp = app;
    }

    public final String[] getArgs() {
        if (args == null) {
            String p = DoJaRuntime.getJam("AppParam");
            Vector v = new Vector();
            if (p != null) {
                int i = 0;
                int n = p.length();
                while (i < n) {
                    while (i < n && p.charAt(i) == ' ') {
                        i++;
                    }
                    int s = i;
                    while (i < n && p.charAt(i) != ' ') {
                        i++;
                    }
                    if (i > s) {
                        v.addElement(p.substring(s, i));
                    }
                }
            }
            args = new String[v.size()];
            v.copyInto(args);
        }
        String[] copy = new String[args.length];
        System.arraycopy(args, 0, copy, 0, args.length);
        return copy;
    }

    public final String getParameter(String key) {
        if (key == null) {
            throw new NullPointerException("key");
        }
        return DoJaRuntime.getJam(key);
    }

    public abstract void start();

    public void resume() {
    }

    public final void terminate() {
        DoJaRuntime.terminate();
    }

    public final String getSourceURL() {
        String u = DoJaRuntime.getJam("PackageURL");
        if (u == null) {
            return "http://localhost/";
        }
        int slash = u.lastIndexOf('/');
        if (u.startsWith("http") && slash > 7) {
            return u.substring(0, slash + 1);
        }
        return "http://localhost/";
    }

    public final int getLaunchType() {
        return LAUNCHED_FROM_MENU;
    }

    public final void launch(int type, String[] largs) {
        System.out.println("DoJa: launch(" + type + ") ignored");
    }

    public int getSuspendInfo() {
        return 0;
    }

    public void setLaunchTime(int kind, ScheduleDate launchTime) {
    }

    public ScheduleDate getLaunchTime(int kind) {
        return null;
    }

    public final boolean isMoved() {
        return false;
    }

    public final void clearMoved() {
    }

    public final boolean isMovedFromOtherTerminal() {
        return false;
    }
}
