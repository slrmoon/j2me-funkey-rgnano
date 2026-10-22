/* Siemens media API (pre-MMAPI), delegating to javax.microedition.media. */
package com.siemens.mp.media;

public interface Controllable {
    Control[] getControls();

    Control getControl(String controlType);
}
