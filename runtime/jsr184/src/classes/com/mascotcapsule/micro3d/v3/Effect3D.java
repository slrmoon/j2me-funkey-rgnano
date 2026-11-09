package com.mascotcapsule.micro3d.v3;

/** Rendering effects: light, shading type, semi-transparency, sphere map. */
public class Effect3D {
    public static final int NORMAL_SHADING = 0;
    public static final int TOON_SHADING = 1;

    Light light;
    int shading = NORMAL_SHADING;
    boolean transparency = true;
    Texture sphere;
    int toonThreshold;
    int toonHigh;
    int toonLow;

    /** no light, normal shading, semi-transparency on, no sphere map */
    public Effect3D() {
    }

    public Effect3D(Light light, int shading, boolean isEnableTrans, Texture tex) {
        checkShading(shading);
        checkSphere(tex);
        this.light = light;
        this.shading = shading;
        transparency = isEnableTrans;
        sphere = tex;
    }

    private static void checkShading(int shading) {
        if (shading != NORMAL_SHADING && shading != TOON_SHADING) {
            throw new IllegalArgumentException("shading");
        }
    }

    private static void checkSphere(Texture tex) {
        if (tex != null && tex.data != null && tex.data.forModel) {
            throw new IllegalArgumentException("model texture as sphere map");
        }
    }

    public final Light getLight() {
        return light;
    }

    public final void setLight(Light light) {
        this.light = light;
    }

    public final int getShadingType() {
        return shading;
    }

    public final void setShadingType(int shading) {
        checkShading(shading);
        this.shading = shading;
    }

    public final int getToonThreshold() {
        return toonThreshold;
    }

    public final int getToonHigh() {
        return toonHigh;
    }

    public final int getToonLow() {
        return toonLow;
    }

    /** levels 0..255: at or above the threshold high, below it low */
    public final void setToonParams(int threshold, int high, int low) {
        if (threshold < 0 || threshold > 255 || high < 0 || high > 255 || low < 0 || low > 255) {
            throw new IllegalArgumentException("toon");
        }
        toonThreshold = threshold;
        toonHigh = high;
        toonLow = low;
    }

    public final boolean isTransparency() {
        return transparency;
    }

    public final void setTransparency(boolean isEnable) {
        transparency = isEnable;
    }

    public final Texture getSphereTexture() {
        return sphere;
    }

    public final void setSphereTexture(Texture tex) {
        checkSphere(tex);
        sphere = tex;
    }

    /** @deprecated */
    public final int getShading() {
        return shading;
    }

    /** @deprecated */
    public final void setShading(int shading) {
        setShadingType(shading);
    }

    /** @deprecated */
    public final int getThreshold() {
        return toonThreshold;
    }

    /** @deprecated */
    public final int getThresholdHigh() {
        return toonHigh;
    }

    /** @deprecated */
    public final int getThresholdLow() {
        return toonLow;
    }

    /** @deprecated */
    public final void setThreshold(int threshold, int high, int low) {
        setToonParams(threshold, high, low);
    }

    /** @deprecated */
    public final boolean isSemiTransparentEnabled() {
        return transparency;
    }

    /** @deprecated */
    public final void setSemiTransparentEnabled(boolean isEnable) {
        transparency = isEnable;
    }

    /** @deprecated */
    public final Texture getSphereMap() {
        return sphere;
    }

    /** @deprecated */
    public final void setSphereMap(Texture tex) {
        setSphereTexture(tex);
    }
}
