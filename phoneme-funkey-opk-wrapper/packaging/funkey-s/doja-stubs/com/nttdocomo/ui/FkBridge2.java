package com.nttdocomo.ui;

/** Access for com.nttdocomo.opt.ui.Graphics2 to Graphics internals. */
public final class FkBridge2 {
    private FkBridge2() {
    }

    public static void setRenderMode(Graphics g, int op, int src, int dst) {
        g.renderOp = op;
        g.renderSrc = src;
        g.renderDst = dst;
    }
}
