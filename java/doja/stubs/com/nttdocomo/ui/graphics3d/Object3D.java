package com.nttdocomo.ui.graphics3d;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public abstract class Object3D {
    public static final int TYPE_NONE = 0;
    public static final int TYPE_ACTION_TABLE = 1;
    public static final int TYPE_FIGURE = 2;
    public static final int TYPE_TEXTURE = 3;
    public static final int TYPE_FOG = 4;
    public static final int TYPE_LIGHT = 5;
    public static final int TYPE_PRIMITIVE = 6;
    public static final int TYPE_GROUP = 7;
    public static final int TYPE_GROUP_MESH = 8;

    private int type;
    private int time;

    protected Object3D(int type) {
        this.type = type;
    }

    protected Object3D() {
    }

    void fkType(int t) {
        type = t;
    }

    public static Object3D createInstance(InputStream inputStream) throws IOException {
        if (inputStream == null) {
            throw new NullPointerException("inputStream");
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buf = new byte[2048];
        int n;
        while ((n = inputStream.read(buf)) > 0) {
            out.write(buf, 0, n);
        }
        Object3D o = createInstance(out.toByteArray());
        if (o == null) {
            throw new IOException("unsupported 3D data");
        }
        return o;
    }

    /** The kind of data is told by its first bytes: BMP texture, MBAC figure, MTRA actions. */
    public static Object3D createInstance(byte[] data) {
        if (data == null) {
            throw new NullPointerException("data");
        }
        if (data.length < 2) {
            return null;
        }
        if (data[0] == 'B' && data[1] == 'M') {
            return new Texture(data);
        }
        try {
            if (data[0] == 'D' && data[1] == '4') {
                StringBuffer head = new StringBuffer();
                for (int i = 0; i < data.length && i < 16; i++) {
                    int b = data[i] & 255;
                    if (i != 0) head.append(' ');
                    if (b < 16) head.append('0');
                    head.append(Integer.toHexString(b));
                }
                com.funkey.doja.Stub.log("Object3D.createInstance: resource=<byte[]> size="
                        + data.length + " head=" + head);
                return D4GroupLoader.load(data);
            }
            if (data[0] == 'M' && data[1] == 'B') {
                return new Figure(data);
            }
            if (data[0] == 'M' && data[1] == 'T') {
                return new ActionTable(data);
            }
        } catch (Exception e) {
            com.funkey.doja.Stub.log("Object3D.createInstance: " + e.getMessage());
            return null;
        }
        com.funkey.doja.Stub.log("Object3D.createInstance: unknown data "
                + (char) data[0] + (char) data[1]);
        return null;
    }

    public void dispose() {
        type = TYPE_NONE;
    }

    public int getType() {
        return type;
    }

    public void setTime(int time) {
        this.time = time;
    }

    public int getTime() {
        return time;
    }
}
