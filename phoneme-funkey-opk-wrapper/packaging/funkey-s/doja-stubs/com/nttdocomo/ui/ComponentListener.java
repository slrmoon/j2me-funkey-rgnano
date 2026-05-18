package com.nttdocomo.ui;

public interface ComponentListener {
    int BUTTON_PRESSED = 1;
    int SELECTION_CHANGED = 2;
    int TEXT_CHANGED = 3;

    void componentAction(Component source, int type, int param);
}
