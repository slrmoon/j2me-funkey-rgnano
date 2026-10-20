/* Siemens media API (pre-MMAPI), delegating to javax.microedition.media. */
package com.siemens.mp.media;

public interface Player extends Controllable {
    int UNREALIZED = 100;
    int REALIZED = 200;
    int PREFETCHED = 300;
    int STARTED = 400;
    int CLOSED = 0;
    long TIME_UNKNOWN = -1L;

    void realize() throws MediaException;
    void prefetch() throws MediaException;
    void start() throws MediaException;
    void stop() throws MediaException;
    void deallocate();
    void close();
    long setMediaTime(long now) throws MediaException;
    long getMediaTime();
    int getState();
    long getDuration();
    String getContentType();
    void setLoopCount(int count);
    void addPlayerListener(PlayerListener listener);
    void removePlayerListener(PlayerListener listener);
}
