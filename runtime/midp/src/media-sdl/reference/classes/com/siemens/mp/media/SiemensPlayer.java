/* Siemens media API: adapter over a javax.microedition.media.Player. */
package com.siemens.mp.media;

import java.util.Vector;

final class SiemensPlayer implements Player, javax.microedition.media.PlayerListener {
    private final javax.microedition.media.Player p;
    private final Vector listeners = new Vector(1);

    SiemensPlayer(javax.microedition.media.Player p) {
        this.p = p;
        p.addPlayerListener(this);
    }

    public void playerUpdate(javax.microedition.media.Player player, String event, Object data) {
        int n = listeners.size();
        for (int i = 0; i < n; i++) {
            try {
                ((PlayerListener) listeners.elementAt(i)).playerUpdate(this, event, data);
            } catch (Throwable t) {
                /* ignore */
            }
        }
    }

    private static MediaException wrap(javax.microedition.media.MediaException e) {
        return new MediaException(e.getMessage());
    }

    public void realize() throws MediaException {
        try {
            p.realize();
        } catch (javax.microedition.media.MediaException e) {
            throw wrap(e);
        }
    }

    public void prefetch() throws MediaException {
        try {
            p.prefetch();
        } catch (javax.microedition.media.MediaException e) {
            throw wrap(e);
        }
    }

    public void start() throws MediaException {
        try {
            p.start();
        } catch (javax.microedition.media.MediaException e) {
            throw wrap(e);
        }
    }

    public void stop() throws MediaException {
        try {
            p.stop();
        } catch (javax.microedition.media.MediaException e) {
            throw wrap(e);
        }
    }

    public void deallocate() {
        p.deallocate();
    }

    public void close() {
        p.close();
    }

    public long setMediaTime(long now) throws MediaException {
        try {
            return p.setMediaTime(now);
        } catch (javax.microedition.media.MediaException e) {
            throw wrap(e);
        }
    }

    public long getMediaTime() {
        return p.getMediaTime();
    }

    public int getState() {
        return p.getState();
    }

    public long getDuration() {
        return p.getDuration();
    }

    public String getContentType() {
        return p.getContentType();
    }

    public void setLoopCount(int count) {
        p.setLoopCount(count);
    }

    public void addPlayerListener(PlayerListener l) {
        if (l != null && !listeners.contains(l)) listeners.addElement(l);
    }

    public void removePlayerListener(PlayerListener l) {
        listeners.removeElement(l);
    }

    public Control[] getControls() {
        return new Control[0];
    }

    public Control getControl(String type) {
        return null;
    }
}
