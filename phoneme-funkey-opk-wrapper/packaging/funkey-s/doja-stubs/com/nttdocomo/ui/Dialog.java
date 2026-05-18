package com.nttdocomo.ui;

public class Dialog {
    public static final int BUTTON_OK = 0;
    private String text;

    public Dialog(int type, String title) {
    }

    public void setText(String value) {
        text = value;
    }

    public int show() {
        System.out.println(text == null ? "" : text);
        return 0;
    }
}
