package com.nttdocomo.ui;

/**
 * Flow layout with HTML-like structure: br() breaks the line before the
 * next added component, begin(P/LEFT/CENTER/RIGHT) ... end() aligns the
 * components added in between.
 */
public class HTMLLayout implements LayoutManager {
    public static final String P = "P";
    public static final String LEFT = "LEFT";
    public static final String CENTER = "CENTER";
    public static final String RIGHT = "RIGHT";

    private final java.util.Vector stack = new java.util.Vector();
    private int pendingBreaks;
    private boolean pendingParagraph;

    public HTMLLayout() {
    }

    public void br() {
        pendingBreaks++;
    }

    public void begin(String tag) {
        if (tag == null) {
            throw new NullPointerException("tag");
        }
        String t = tag.toUpperCase();
        if (!P.equals(t) && !LEFT.equals(t) && !CENTER.equals(t) && !RIGHT.equals(t)) {
            throw new IllegalArgumentException("tag");
        }
        stack.addElement(t);
        if (P.equals(t)) {
            pendingParagraph = true;
        }
    }

    public void end() {
        if (stack.isEmpty()) {
            throw new UIException(UIException.ILLEGAL_STATE, "end() without begin()");
        }
        String t = (String) stack.lastElement();
        stack.removeElementAt(stack.size() - 1);
        if (P.equals(t)) {
            pendingParagraph = true;
        }
    }

    /** Panel.add: what the new component gets. */
    void stamp(Component c) {
        c.htmlBreaks = pendingBreaks + (pendingParagraph ? 1 : 0);
        pendingBreaks = 0;
        pendingParagraph = false;
        int align = 0;
        for (int i = stack.size() - 1; i >= 0; i--) {
            String t = (String) stack.elementAt(i);
            if (CENTER.equals(t)) {
                align = 1;
                break;
            }
            if (RIGHT.equals(t)) {
                align = 2;
                break;
            }
            if (LEFT.equals(t)) {
                break;
            }
        }
        c.htmlAlign = align;
    }
}
