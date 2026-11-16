/* LG MMPP media player (mmpp.media), over javax.microedition.media: a sound from the jar
 * ("/snd/bgm.mid") or a file name, started and stopped by the game. */
package mmpp.media;

import java.io.InputStream;
import javax.microedition.media.Manager;
import javax.microedition.media.Player;
import javax.microedition.media.control.VolumeControl;

public class MediaPlayer {
    private Player player;
    private String location;
    private boolean loop;
    private int volume = 100;

    public MediaPlayer() {
    }

    private static String typeOf(String name) {
        String n = name.toLowerCase();
        if (n.endsWith(".mid") || n.endsWith(".midi")) return "audio/midi";
        if (n.endsWith(".wav")) return "audio/x-wav";
        if (n.endsWith(".amr")) return "audio/amr";
        if (n.endsWith(".mp3")) return "audio/mpeg";
        if (n.endsWith(".mmf") || n.endsWith(".smaf")) return "application/vnd.smaf";
        return "audio/midi";
    }

    private void close() {
        if (player != null) {
            try {
                player.close();
            } catch (Throwable t) {
            }
            player = null;
        }
    }

    public void setMediaLocation(String location) {
        close();
        this.location = location;
    }

    public void setMediaSource(byte[] data, int offset, int length) {
        close();
        try {
            byte[] b = new byte[length];
            System.arraycopy(data, offset, b, 0, length);
            player = Manager.createPlayer(new java.io.ByteArrayInputStream(b), "audio/midi");
        } catch (Throwable t) {
            player = null;
        }
        location = null;
    }

    private boolean open() {
        if (player != null) return true;
        if (location == null) return false;
        try {
            String loc = location.startsWith("/") ? location : "/" + location;
            InputStream in = getClass().getResourceAsStream(loc);
            if (in == null) return false;
            player = Manager.createPlayer(in, typeOf(loc));
            player.realize();
            player.prefetch();
            return true;
        } catch (Throwable t) {
            player = null;
            return false;
        }
    }

    public void start() {
        if (!open()) return;
        try {
            player.stop();
            player.setMediaTime(0);
            player.setLoopCount(loop ? -1 : 1);
            VolumeControl vc = (VolumeControl) player.getControl("VolumeControl");
            if (vc != null) vc.setLevel(volume);
            player.start();
        } catch (Throwable t) {
        }
    }

    public void stop() {
        if (player == null) return;
        try {
            player.stop();
        } catch (Throwable t) {
        }
    }

    public void pause() {
        stop();
    }

    public void resume() {
        if (player == null) return;
        try {
            player.start();
        } catch (Throwable t) {
        }
    }

    public void setPlayBackLoop(boolean loop) {
        this.loop = loop;
    }

    public boolean isPlayBackLoop() {
        return loop;
    }

    /* LG levels are "0".."5" (some games pass a percentage) */
    public void setVolumeLevel(String level) {
        try {
            int v = Integer.parseInt(level.trim());
            volume = v <= 5 ? v * 20 : Math.min(100, v);
        } catch (Throwable t) {
        }
    }

    public String getVolumeLevel() {
        return String.valueOf(volume / 20);
    }
}
