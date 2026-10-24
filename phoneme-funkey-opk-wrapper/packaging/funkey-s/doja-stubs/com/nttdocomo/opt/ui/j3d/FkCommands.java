package com.nttdocomo.opt.ui.j3d;

/** Graphics3D command lists (executeCommandList). */
final class FkCommands {
    private FkCommands() {
    }

    private static boolean warned;

    static void run(FkRender r, int[] cmds) {
        if (cmds == null) {
            throw new NullPointerException("commands");
        }
        if (!warned) {
            warned = true;
            System.out.println("DoJa: unimplemented Graphics3D.executeCommandList");
        }
    }
}
