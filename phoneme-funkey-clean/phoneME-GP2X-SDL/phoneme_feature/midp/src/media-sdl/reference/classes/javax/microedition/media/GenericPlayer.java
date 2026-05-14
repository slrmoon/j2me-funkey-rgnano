/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */
package javax.microedition.media;

import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;

import javax.microedition.media.control.VolumeControl;

public class GenericPlayer extends ABBBasicPlayer implements Runnable, VolumeControl {
    private static final int SAMPLE_FORMAT_WAV = 1;
    private static final int SAMPLE_FORMAT_AMR = 2;

    private native int nSamplePlayerInit();
    private native int nSamplePlayerRealize(int id, byte[] buf, int format);
    private native int nSamplePlayerPrefetch(int id);
    private native int nSamplePlayerStart(int id);
    private native void nSamplePlayerStop(int id);
    private native void nSamplePlayerDeallocate(int id);
    private native void nSamplePlayerClose(int id);
    private native long nSampleGetMediaTime(int id);
    private native int nSampleCheckEOM(int id);
    private native void nSampleSetVolumeLevel(int id, int value);

    private String genericPlayerType;
    private Thread checkThread;
    private int samplePlayerId;
    private int volumeLevel;

    public GenericPlayer(String playerType) {
        super();
        try {
            Manager.ensureAudioSubsystem();
        } catch (MediaException e) {
            throw new IllegalStateException(e.getMessage());
        }
        genericPlayerType = playerType;
        System.out.println("GenericPlayer.<init> type=" + genericPlayerType);
        checkThread = null;
        volumeLevel = 100;
        samplePlayerId = 0;
        if (isSampleType()) {
            samplePlayerId = nSamplePlayerInit();
            if (samplePlayerId == 0) {
                throw new IllegalStateException("Out Of Memory Error!!!");
            }
        }
    }

    public int getAudioType() {
        return AUDIO_PCM;
    }

    protected void doRealize() throws MediaException {
        System.out.println("GenericPlayer.doRealize type=" + genericPlayerType + " sample=" + isSampleType());
        if (!isSampleType()) {
            return;
        }
        try {
            byte[] buffer = readAll(source);
            if (nSamplePlayerRealize(samplePlayerId, buffer, getSampleFormat()) != 0) {
                throw new MediaException("Audio realize error");
            }
        } catch (IOException e) {
            throw new MediaException(e.getMessage());
        }
    }

    protected void doPrefetch() throws MediaException {
        System.out.println("GenericPlayer.doPrefetch type=" + genericPlayerType + " sample=" + isSampleType());
        if (!isSampleType()) {
            return;
        }
        if (nSamplePlayerPrefetch(samplePlayerId) != 0) {
            throw new MediaException("Audio prefetch error");
        }
    }

    protected boolean doStart() {
        System.out.println("GenericPlayer.doStart type=" + genericPlayerType + " sample=" + isSampleType());
        if (!isSampleType()) {
            return true;
        }
        return nSamplePlayerStart(samplePlayerId) == 0;
    }

    protected void doPostStart() {
        if (!isSampleType()) {
            return;
        }
        if (checkThread == null || !checkThread.isAlive()) {
            checkThread = new Thread(this);
            checkThread.start();
        }
    }

    protected void doStop() throws MediaException {
        if (isSampleType()) {
            nSamplePlayerStop(samplePlayerId);
        }
    }

    protected void doDeallocate() {
        if (isSampleType()) {
            nSamplePlayerDeallocate(samplePlayerId);
        }
    }

    protected void doClose() {
        if (isSampleType() && samplePlayerId != 0) {
            nSamplePlayerClose(samplePlayerId);
            samplePlayerId = 0;
        }
    }

    protected long doSetMediaTime(long now) throws MediaException {
        return now;
    }

    protected long doGetMediaTime() {
        if (!isSampleType()) {
            return TIME_UNKNOWN;
        }
        return nSampleGetMediaTime(samplePlayerId);
    }

    protected long doGetDuration() {
        return TIME_UNKNOWN;
    }

    protected Control doGetControl(String type) {
        if (type.compareTo("javax.microedition.media.control.VolumeControl") == 0 &&
                isSampleType()) {
            return this;
        }
        return null;
    }

    public String getContentType() {
        chkClosed(true);
        return genericPlayerType;
    }

    public void run() {
        while (state == Player.STARTED) {
            if (nSampleCheckEOM(samplePlayerId) != 0) {
                sendEvent(PlayerListener.END_OF_MEDIA, new Long(doGetMediaTime()));
                return;
            }
            Thread.yield();
        }
    }

    public void setMute(boolean mute) {
        if (mute) {
            volumeLevel = 0;
            if (isSampleType()) {
                nSampleSetVolumeLevel(samplePlayerId, volumeLevel);
            }
        }
    }

    public boolean isMuted() {
        return volumeLevel == 0;
    }

    public int setLevel(int level) {
        if (level < 0) {
            level = 0;
        }
        if (level > 100) {
            level = 100;
        }
        volumeLevel = level;
        if (isSampleType()) {
            nSampleSetVolumeLevel(samplePlayerId, volumeLevel);
        }
        return volumeLevel;
    }

    public int getLevel() {
        return volumeLevel;
    }

    private boolean isWavType() {
        return "audio/wav".equalsIgnoreCase(genericPlayerType) ||
                "audio/x-wav".equalsIgnoreCase(genericPlayerType);
    }

    private boolean isAmrType() {
        return "audio/amr".equalsIgnoreCase(genericPlayerType) ||
                "audio/amr-nb".equalsIgnoreCase(genericPlayerType);
    }

    private boolean isSampleType() {
        return isWavType() || isAmrType();
    }

    private int getSampleFormat() {
        if (isAmrType()) {
            return SAMPLE_FORMAT_AMR;
        }
        return SAMPLE_FORMAT_WAV;
    }

    private byte[] readAll(InputStream in) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] tmp = new byte[4096];
        int read;
        while ((read = in.read(tmp)) != -1) {
            out.write(tmp, 0, read);
        }
        return out.toByteArray();
    }
}
