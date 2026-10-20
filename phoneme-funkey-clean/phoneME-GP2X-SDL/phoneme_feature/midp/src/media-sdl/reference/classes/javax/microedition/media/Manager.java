/*
 *   
 *
 *  Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version
 *  2 only, as published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License version 2 for more details (a copy is
 *  included at /legal/license.txt).
 *  
 *  You should have received a copy of the GNU General Public License
 *  version 2 along with this work; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA
 *  
 *  Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 *  Clara, CA 95054 or visit www.sun.com if you need additional
 *  information or have any questions.
 */

package javax.microedition.media;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

import javax.microedition.io.Connector;
import javax.microedition.media.protocol.DataSource;
import javax.microedition.media.protocol.SourceStream;

/**
 * <code>Manager</code> is the access point for obtaining system dependent
 * resources such as <code>Player</code>s for multimedia processing.
 * <p>
 * FunKey implementation: every player is played by the fkaudio engine
 * (Sonivox EAS synthesizer for MIDI and melody formats, a 22050 Hz mixer
 * for sampled audio). The content format is detected from the data itself;
 * the declared content type is only a hint, because games often get it
 * wrong.
 */
public final class Manager {
    /** The locator to create a tone <code>Player</code>. */
    public final static String TONE_DEVICE_LOCATOR = "device://tone";

    /** The locator to create a MIDI <code>Player</code>. */
    public final static String MIDI_DEVICE_LOCATOR = "device://midi";

    private static final String[] CONTENT_TYPES = {
        "audio/midi", "audio/x-midi", "audio/mid", "audio/sp-midi",
        "audio/x-tone-seq",
        "audio/x-wav", "audio/wav", "audio/basic", "audio/x-au",
        "audio/amr", "audio/amr-nb",
        "audio/mpeg", "audio/mp3", "audio/x-mp3",
        "application/vnd.smaf", "application/x-smaf", "audio/mmf",
        "audio/imelody", "text/x-imelody", "audio/x-imelody",
        "audio/x-nokia-ota", "application/vnd.nokia.ringing-tone",
        "audio/x-rtttl", "audio/rtttl"
    };

    private static final String[] PROTOCOLS = {"device", "file", "http", "https", "resource"};

    private static final TimeBase SYSTEM_TIME_BASE = new TimeBase() {
        public long getTime() {
            return System.currentTimeMillis() * 1000L;
        }
    };

    private static boolean audioReady;

    private Manager() {
    }

    /**
     * Return the list of supported content types for the given protocol.
     * @param protocol the input protocol, or <code>null</code> for all
     * @return the list of supported content types
     */
    public static String[] getSupportedContentTypes(String protocol) {
        if (protocol != null) {
            boolean known = false;
            for (int i = 0; i < PROTOCOLS.length; i++) {
                if (PROTOCOLS[i].equals(protocol)) known = true;
            }
            if (!known) return new String[0];
            if (protocol.equals("device")) return new String[] {"audio/x-tone-seq", "audio/midi"};
        }
        String[] r = new String[CONTENT_TYPES.length];
        System.arraycopy(CONTENT_TYPES, 0, r, 0, r.length);
        return r;
    }

    /**
     * Return the list of supported protocols given the content type.
     * @param content_type the content type, or <code>null</code> for all
     * @return the list of supported protocols
     */
    public static String[] getSupportedProtocols(String content_type) {
        if (content_type != null) {
            boolean known = false;
            for (int i = 0; i < CONTENT_TYPES.length; i++) {
                if (CONTENT_TYPES[i].equalsIgnoreCase(content_type)) known = true;
            }
            if (!known) return new String[0];
        }
        String[] r = new String[PROTOCOLS.length];
        System.arraycopy(PROTOCOLS, 0, r, 0, r.length);
        return r;
    }

    static synchronized void ensureAudio() throws MediaException {
        if (!audioReady) {
            int r = FkaNative.nInit();
            if (r != 0) throw new MediaException("Cannot initialize audio (" + r + ")");
            audioReady = true;
        }
    }

    /**
     * Create a <code>Player</code> from an input locator.
     * @param locator a locator string in URI syntax
     * @return a new <code>Player</code>
     * @exception MediaException if a <code>Player</code> cannot be created
     * @exception IOException if the source cannot be read
     */
    public static Player createPlayer(String locator) throws IOException, MediaException {
        if (locator == null) throw new IllegalArgumentException("null locator");
        ensureAudio();
        String l = locator.trim();
        if (l.equals(TONE_DEVICE_LOCATOR)) return FkaPlayer.toneDevice();
        if (l.equals(MIDI_DEVICE_LOCATOR)) return FkaPlayer.midiDevice();
        if (l.startsWith("capture:")) throw new MediaException("Capture is not supported");
        if (l.startsWith("device:")) throw new MediaException("Unknown device: " + l);
        if (l.indexOf(':') < 0 && !l.startsWith("/")) {
            throw new MediaException("Malformed locator: " + l);
        }
        InputStream in = null;
        try {
            if (l.startsWith("resource:") || l.startsWith("/")) {
                String path = l.startsWith("resource:") ? l.substring(9) : l;
                if (path.startsWith("//")) path = path.substring(1);
                if (!path.startsWith("/")) path = "/" + path;
                /* no class literal: javac -target 1.4 turns it into a
                 * helper calling Throwable.initCause, absent in CLDC; the
                 * lookup of an absolute path does not depend on the class */
                in = "".getClass().getResourceAsStream(path);
                if (in == null) throw new IOException("Resource not found: " + path);
            } else {
                in = Connector.openInputStream(l);
            }
            byte[] data = readAll(in);
            return FkaPlayer.fromContent(data, data.length, typeFromLocator(l));
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {
                    /* ignore */
                }
            }
        }
    }

    /**
     * Create a <code>Player</code> to play back media from an
     * <code>InputStream</code>. The whole stream is read here, so the
     * caller may close it right after this call.
     * @param stream the <code>InputStream</code> that delivers the media
     * @param type the content type, or <code>null</code> to detect it
     * @return a new <code>Player</code>
     * @exception MediaException if the content is not supported
     * @exception IOException if the stream cannot be read
     */
    public static Player createPlayer(InputStream stream, String type)
            throws IOException, MediaException {
        if (stream == null) throw new IllegalArgumentException("null stream");
        ensureAudio();
        byte[] data = readAll(stream);
        return FkaPlayer.fromContent(data, data.length, type);
    }

    /**
     * Create a <code>Player</code> for a <code>DataSource</code>.
     * @param source the <code>DataSource</code> that provides the media
     * @return a new <code>Player</code>
     * @exception MediaException if the content is not supported
     * @exception IOException if the source cannot be read
     */
    public static Player createPlayer(DataSource source) throws IOException, MediaException {
        if (source == null) throw new IllegalArgumentException("null source");
        ensureAudio();
        source.connect();
        try {
            source.start();
            SourceStream[] streams = source.getStreams();
            if (streams == null || streams.length == 0) throw new MediaException("No streams");
            SourceStream s = streams[0];
            ByteArrayOutputStream out = new ByteArrayOutputStream(16384);
            byte[] buf = new byte[4096];
            int n;
            while ((n = s.read(buf, 0, buf.length)) > 0) out.write(buf, 0, n);
            byte[] data = out.toByteArray();
            return FkaPlayer.fromContent(data, data.length, source.getContentType());
        } finally {
            source.disconnect();
        }
    }

    /**
     * Play back a tone as specified by a note and its duration.
     * @param note the note, 0..127 (69 is A4, 440 Hz)
     * @param duration the duration in milliseconds, positive
     * @param volume the volume, 0..100
     * @exception MediaException if the tone cannot be played
     */
    public static void playTone(int note, int duration, int volume) throws MediaException {
        if (note < 0 || note > 127 || duration <= 0) {
            throw new IllegalArgumentException("Bad note or duration");
        }
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;
        ensureAudio();
        FkaNative.nPlayTone(note, duration, volume);
    }

    /**
     * Get the time-base object for the system.
     * @return the system time base
     */
    public static TimeBase getSystemTimeBase() {
        return SYSTEM_TIME_BASE;
    }

    /* ---------------------------------------------------------------- */

    static byte[] readAll(InputStream in) throws IOException {
        int avail = 0;
        try {
            avail = in.available();
        } catch (IOException e) {
            /* some streams do not know */
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream(avail > 0 ? avail + 1 : 16384);
        byte[] buf = new byte[avail > 4096 ? Math.min(avail + 1, 65536) : 4096];
        int n;
        while ((n = in.read(buf, 0, buf.length)) != -1) {
            if (n == 0) {
                /* misbehaving stream: read a byte to make progress */
                int b = in.read();
                if (b < 0) break;
                out.write(b);
            } else {
                out.write(buf, 0, n);
            }
        }
        return out.toByteArray();
    }

    private static String typeFromLocator(String locator) {
        int q = locator.indexOf('?');
        if (q >= 0) locator = locator.substring(0, q);
        int dot = locator.lastIndexOf('.');
        if (dot < 0 || dot == locator.length() - 1) return null;
        String ext = locator.substring(dot + 1).toLowerCase();
        if (ext.equals("mid") || ext.equals("midi") || ext.equals("rmi") || ext.equals("kar")) return "audio/midi";
        if (ext.equals("wav") || ext.equals("wave")) return "audio/x-wav";
        if (ext.equals("au") || ext.equals("snd")) return "audio/basic";
        if (ext.equals("amr")) return "audio/amr";
        if (ext.equals("mp3")) return "audio/mpeg";
        if (ext.equals("mmf") || ext.equals("smaf")) return "application/vnd.smaf";
        if (ext.equals("imy")) return "audio/imelody";
        if (ext.equals("ott") || ext.equals("ota") || ext.equals("rng")) return "audio/x-nokia-ota";
        if (ext.equals("rtttl") || ext.equals("rtx") || ext.equals("nok")) return "audio/x-rtttl";
        if (ext.equals("jts")) return "audio/x-tone-seq";
        return null;
    }
}
