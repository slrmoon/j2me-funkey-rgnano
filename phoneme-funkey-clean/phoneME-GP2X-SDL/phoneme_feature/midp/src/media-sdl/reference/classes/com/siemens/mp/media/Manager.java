/* Siemens media API (pre-MMAPI), delegating to javax.microedition.media. */
package com.siemens.mp.media;

import java.io.IOException;
import java.io.InputStream;

import com.siemens.mp.media.protocol.DataSource;

public final class Manager {
    private Manager() {
    }

    public static String[] getSupportedContentTypes(String protocol) {
        return javax.microedition.media.Manager.getSupportedContentTypes(protocol);
    }

    public static String[] getSupportedProtocols(String contentType) {
        return javax.microedition.media.Manager.getSupportedProtocols(contentType);
    }

    public static Player createPlayer(String locator) throws IOException, MediaException {
        try {
            return new SiemensPlayer(javax.microedition.media.Manager.createPlayer(locator));
        } catch (javax.microedition.media.MediaException e) {
            throw new MediaException(e.getMessage());
        }
    }

    public static Player createPlayer(InputStream stream, String type) throws IOException, MediaException {
        try {
            return new SiemensPlayer(javax.microedition.media.Manager.createPlayer(stream, type));
        } catch (javax.microedition.media.MediaException e) {
            throw new MediaException(e.getMessage());
        }
    }

    public static Player createPlayer(DataSource source) throws IOException, MediaException {
        throw new MediaException("DataSource not supported");
    }

    public static void playTone(int note, int duration, int volume) throws MediaException {
        try {
            javax.microedition.media.Manager.playTone(note, duration, volume);
        } catch (javax.microedition.media.MediaException e) {
            throw new MediaException(e.getMessage());
        }
    }
}
