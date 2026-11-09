/* Siemens media API (pre-MMAPI), delegating to javax.microedition.media. */
package com.siemens.mp.media;

public class MediaException extends Exception {
    public MediaException() {
    }

    public MediaException(String reason) {
        super(reason);
    }

    public MediaException(Exception e) {
        super(e == null ? null : e.getMessage());
    }
}
