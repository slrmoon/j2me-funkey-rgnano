/* Siemens media API volume control (same methods as MMAPI's). */
package com.siemens.mp.media.control;

public interface VolumeControl extends com.siemens.mp.media.Control {
    void setMute(boolean mute);

    boolean isMuted();

    int setLevel(int level);

    int getLevel();
}
