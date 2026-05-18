package com.nttdocomo.ui;

import com.funkey.doja.DoJaRuntime;

public final class Display {
    public static final int KEY_PRESSED_EVENT = 0;
    public static final int KEY_RELEASED_EVENT = 1;
    public static final int RESUME_VM_EVENT = 4;
    public static final int UPDATE_VM_EVENT = 6;
    public static final int TIMER_EXPIRED_EVENT = 7;

    public static final int KEY_0 = 0x00;
    public static final int KEY_1 = 0x01;
    public static final int KEY_2 = 0x02;
    public static final int KEY_3 = 0x03;
    public static final int KEY_4 = 0x04;
    public static final int KEY_5 = 0x05;
    public static final int KEY_6 = 0x06;
    public static final int KEY_7 = 0x07;
    public static final int KEY_8 = 0x08;
    public static final int KEY_9 = 0x09;
    public static final int KEY_ASTERISK = 0x0a;
    public static final int KEY_POUND = 0x0b;
    public static final int KEY_LEFT = 0x10;
    public static final int KEY_UP = 0x11;
    public static final int KEY_RIGHT = 0x12;
    public static final int KEY_DOWN = 0x13;
    public static final int KEY_SELECT = 0x14;
    public static final int KEY_SOFT1 = 0x15;
    public static final int KEY_SOFT2 = 0x16;
    public static final int KEY_CLEAR = 0x20;

    private Display() {
    }

    public static int getWidth() {
        javax.microedition.lcdui.Displayable d = DoJaRuntime.getMIDPDisplay().getCurrent();
        return d == null ? 240 : d.getWidth();
    }

    public static int getHeight() {
        javax.microedition.lcdui.Displayable d = DoJaRuntime.getMIDPDisplay().getCurrent();
        return d == null ? 240 : d.getHeight();
    }

    public static void setCurrent(Frame frame) {
        DoJaRuntime.getMIDPDisplay().setCurrent(frame);
    }

    static int mapMIDPKeyCode(int keyCode) {
        switch (keyCode) {
        case -1:
        case -17:
            return KEY_UP;
        case -2:
        case -18:
            return KEY_DOWN;
        case -3:
        case -19:
            return KEY_LEFT;
        case -4:
        case -20:
            return KEY_RIGHT;
        case -5:
        case -13:
            return KEY_SELECT;
        case -6:
            return KEY_SOFT1;
        case -7:
            return KEY_SOFT2;
        case -8:
            return KEY_CLEAR;
        case '*':
            return KEY_ASTERISK;
        case '#':
            return KEY_POUND;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return keyCode - '0';
        default:
            return keyCode;
        }
    }
}
