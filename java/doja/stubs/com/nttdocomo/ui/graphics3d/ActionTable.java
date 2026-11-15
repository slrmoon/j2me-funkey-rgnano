package com.nttdocomo.ui.graphics3d;

import com.nttdocomo.opt.ui.j3d.FkMesh;

/** The actions of a figure (MTRA data); frames are 16.16 numbers. */
public class ActionTable extends Object3D {
    com.nttdocomo.opt.ui.j3d.ActionTable fkTable;

    ActionTable(byte[] data) {
        super(TYPE_ACTION_TABLE);
        fkTable = FkMesh.loadActions(data);
    }

    public int getNumActions() {
        return FkMesh.numActions(fkTable);
    }

    public int getMaxFrame(int action) {
        return FkMesh.maxFrame(fkTable, action);
    }
}
