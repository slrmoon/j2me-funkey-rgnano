package com.nttdocomo.ui;

import com.funkey.doja.DoJaRuntime;

public class Display {
    public static final int KEY_PRESSED_EVENT = 0;
    public static final int KEY_RELEASED_EVENT = 1;
    public static final int RESUME_VM_EVENT = 4;
    public static final int RESET_VM_EVENT = 5;
    public static final int UPDATE_VM_EVENT = 6;
    public static final int TIMER_EXPIRED_EVENT = 7;
    public static final int MEDIA_EVENT = 8;
    protected static final int MIN_VENDOR_EVENT = 64;
    protected static final int MAX_VENDOR_EVENT = 127;
    public static final int POINTER_MOVED_EVENT = 64;
    public static final int FINGER_MOVED_EVENT = 65;

    public static final int KEY_0 = 0;
    public static final int KEY_1 = 1;
    public static final int KEY_2 = 2;
    public static final int KEY_3 = 3;
    public static final int KEY_4 = 4;
    public static final int KEY_5 = 5;
    public static final int KEY_6 = 6;
    public static final int KEY_7 = 7;
    public static final int KEY_8 = 8;
    public static final int KEY_9 = 9;
    public static final int KEY_ASTERISK = 10;
    public static final int KEY_POUND = 11;
    public static final int KEY_LEFT = 16;
    public static final int KEY_UP = 17;
    public static final int KEY_RIGHT = 18;
    public static final int KEY_DOWN = 19;
    public static final int KEY_SELECT = 20;
    public static final int KEY_SOFT1 = 21;
    public static final int KEY_SOFT2 = 22;
    public static final int KEY_IAPP = 24;
    public static final int KEY_UPPER_LEFT = 26;
    public static final int KEY_UPPER_RIGHT = 27;
    public static final int KEY_LOWER_RIGHT = 28;
    public static final int KEY_LOWER_LEFT = 29;
    public static final int KEY_PAGE_UP = 30;
    public static final int KEY_PAGE_DOWN = 31;
    public static final int KEY_CLEAR = 32;
    public static final int KEY_MAIL = 33;
    public static final int KEY_MEMO = 34;
    public static final int KEY_MENU = 35;
    public static final int KEY_I_MODE = 36;
    public static final int KEY_PHONE_BOOK = 37;
    public static final int KEY_CALENDAR = 38;
    public static final int KEY_VOICE = 39;
    public static final int KEY_MANNER_MODE = 40;
    public static final int KEY_DRIVE_MODE = 41;
    public static final int KEY_GPS = 42;
    public static final int KEY_ROLL_LEFT = 48;
    public static final int KEY_ROLL_RIGHT = 49;
    public static final int KEY_SUB1 = 50;
    public static final int KEY_SUB2 = 51;
    public static final int KEY_SUB3 = 52;
    public static final int KEY_MY_SELECT = 53;
    public static final int KEY_CAMERA = 56;
    public static final int KEY_CAMERA_ZOOM_IN = 57;
    public static final int KEY_CAMERA_ZOOM_OUT = 58;
    public static final int KEY_CAMERA_SELECT = 59;
    public static final int KEY_CAMERA_LIGHT = 60;
    public static final int KEY_CAMERA_SHOT = 61;
    protected static final int MIN_VENDOR_KEY = 0x1a;
    protected static final int MAX_VENDOR_KEY = 0x1f;
    protected static final int MIN_OPTION_KEY = 0x1a;
    protected static final int MAX_OPTION_KEY = 0x3f;

    private static Frame current;

    protected Display() {
    }

    public static Frame getCurrent() {
        return current;
    }

    public static int getWidth() {
        return DoJaRuntime.drawWidth();
    }

    public static int getHeight() {
        return DoJaRuntime.drawHeight();
    }

    public static boolean isColor() {
        return true;
    }

    public static int numColors() {
        return 65536;
    }

    public static void setCurrent(Frame frame) {
        if (frame == null) {
            throw new NullPointerException("frame");
        }
        if (frame instanceof Dialog) {
            throw new IllegalArgumentException("dialog");
        }
        if (current == frame) {
            return;
        }
        current = frame;
        final Frame f = frame;
        DoJaRuntime.postRun(new Runnable() {
            public void run() {
                if (current == f) {
                    f.fkShown();
                }
            }
        });
        DoJaRuntime.frameChanged();
    }
}
