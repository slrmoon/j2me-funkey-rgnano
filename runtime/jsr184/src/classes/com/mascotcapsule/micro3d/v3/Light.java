package com.mascotcapsule.micro3d.v3;

/**
 * Parallel (directional) light and ambient light; intensities 4096 = 100%.
 * The direction is the way the light travels.
 */
public class Light {
    private Vector3D direction;
    private int dirIntensity;
    private int ambIntensity;

    public Light() {
        this(new Vector3D(0, 0, 4096), 4096, 0);
    }

    public Light(Vector3D dir, int dirIntensity, int ambIntensity) {
        if (dir == null) {
            throw new NullPointerException();
        }
        direction = dir;
        this.dirIntensity = dirIntensity;
        this.ambIntensity = ambIntensity;
    }

    public final int getParallelLightIntensity() {
        return dirIntensity;
    }

    public final void setParallelLightIntensity(int p) {
        dirIntensity = p;
    }

    public final int getAmbientIntensity() {
        return ambIntensity;
    }

    public final void setAmbientIntensity(int p) {
        ambIntensity = p;
    }

    public final Vector3D getParallelLightDirection() {
        return direction;
    }

    public final void setParallelLightDirection(Vector3D v) {
        if (v == null) {
            throw new NullPointerException();
        }
        direction = v;
    }

    /** @deprecated */
    public final int getDirIntensity() {
        return dirIntensity;
    }

    /** @deprecated */
    public final void setDirIntensity(int p) {
        dirIntensity = p;
    }

    /** @deprecated */
    public final int getAmbIntensity() {
        return ambIntensity;
    }

    /** @deprecated */
    public final void setAmbIntensity(int p) {
        ambIntensity = p;
    }

    /** @deprecated */
    public final Vector3D getDirection() {
        return direction;
    }

    /** @deprecated */
    public final void setDirection(Vector3D v) {
        setParallelLightDirection(v);
    }
}
