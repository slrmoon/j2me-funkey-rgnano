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

    public static int renderOp(Graphics g) {
        return g.renderOp;
    }

    public static int renderSrc(Graphics g) {
        return g.renderSrc;
    }

    public static int renderDst(Graphics g) {
        return g.renderDst;
    }

    public static int flipMode(Graphics g) {
        return g.flipMode;
    }
}
