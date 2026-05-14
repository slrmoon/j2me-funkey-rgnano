package com.nokia.mid.sound;

public class Sound {
    public static final int FORMAT_TONE = 1;
    public static final int FORMAT_WAV = 5;

    public static final int SOUND_UNINITIALIZED = 0;
    public static final int SOUND_STOPPED = 1;
    public static final int SOUND_PLAYING = 2;

    private static final short[] FREQ_TABLE = {
        0, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23,
        24, 26, 27, 29, 30, 32, 34, 36, 38, 41, 43, 45, 48, 51, 54, 57, 60,
        64, 68, 72, 76, 81, 85, 90, 96, 101, 107, 114, 120, 128, 135, 143,
        152, 161, 170, 180, 191, 202, 214, 227, 240, 255, 270, 286, 303, 321,
        340, 360, 381, 404, 428, 453, 480, 509, 539, 571, 605, 641, 679, 719,
        762, 807, 855, 906, 960, 1017, 1078, 1142, 1210, 1282, 1358, 1438,
        1524, 1614, 1710, 1812, 1920, 2034, 2155, 2283, 2419, 2563, 2715,
        2876, 3047, 3228, 3420, 3624, 3839, 4067, 4309, 4565, 4837, 5125,
        5429, 5752, 6094, 6456, 6840, 7247, 7678, 8134, 8618, 9130, 9673,
        10249, 10858, 11504, 12188, 12912
    };

    private int state = SOUND_STOPPED;
    private int gain = 100;
    private SoundListener listener;
    private byte[] data;
    private int type;
    private int frequency;
    private long duration;
    private volatile boolean stopRequested;
    private Thread playbackThread;

    public Sound(byte[] data, int type) {
        init(data, type);
    }

    public Sound(int frequency, long duration) {
        init(frequency, duration);
    }

    public void init(byte[] data, int type) {
        this.data = data;
        this.type = type;
        this.frequency = 0;
        this.duration = 0;
        state = SOUND_STOPPED;
    }

    public void init(int frequency, long duration) {
        this.frequency = frequency;
        this.duration = duration;
        this.data = null;
        this.type = FORMAT_TONE;
        state = SOUND_STOPPED;
    }

    public void play(int loop) {
        final int requestedLoop = loop;
        stop();
        state = SOUND_PLAYING;
        stopRequested = false;
        playbackThread = new Thread(new Runnable() {
            public void run() {
                playImpl(requestedLoop);
            }
        });
        playbackThread.start();
        notifyListener();
    }

    public void stop() {
        stopRequested = true;
        state = SOUND_STOPPED;
        notifyListener();
    }

    public void resume() {
        play(1);
    }

    public int getState() {
        return state;
    }

    public int getGain() {
        return gain;
    }

    public void setGain(int gain) {
        this.gain = gain;
    }

    public void setSoundListener(SoundListener listener) {
        this.listener = listener;
    }

    private void playImpl(int loop) {
        int repeats = (loop <= 0) ? Integer.MAX_VALUE : loop;
        int note;
        int toneDuration;
        int volume;
        int i;

        volume = clampGain(gain);

        if (data != null && data.length > 0) {
            note = 48 + (checksum(data) % 24);
            toneDuration = 60 + ((data.length > 1 ? data[1] & 0xFF : data[0] & 0xFF) % 180);
        } else {
            note = convertFreqToNote(frequency > 0 ? frequency : 1760);
            toneDuration = (duration > 0 && duration < 2000) ? (int)duration : 100;
        }

        for (i = 0; i < repeats && !stopRequested; i++) {
            try {
                javax.microedition.media.Manager.playTone(note, toneDuration, volume);
                sleepQuietly(toneDuration + 20);
            } catch (Throwable t) {
                break;
            }
        }

        if (!stopRequested) {
            state = SOUND_STOPPED;
            notifyListener();
        }
    }

    private static int checksum(byte[] bytes) {
        int sum = 0;
        int i;
        for (i = 0; i < bytes.length; i++) {
            sum = (sum + (bytes[i] & 0xFF)) & 0x7FFFFFFF;
        }
        return sum;
    }

    private static int clampGain(int value) {
        if (value < 0) {
            return 0;
        }
        if (value > 100) {
            return 100;
        }
        return value;
    }

    private static void sleepQuietly(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException e) {
        }
    }

    private static int convertFreqToNote(int freq) {
        int low = 0;
        int high = FREQ_TABLE.length - 1;
        int mid;
        int midVal;

        if (freq <= 0) {
            return 69;
        }

        while (low <= high) {
            mid = (low + high) >>> 1;
            midVal = FREQ_TABLE[mid];

            if (midVal < freq) {
                low = mid + 1;
            } else if (midVal > freq) {
                high = mid - 1;
            } else {
                return mid;
            }
        }

        if (low <= 0) {
            return 0;
        }
        if (low >= FREQ_TABLE.length) {
            return FREQ_TABLE.length - 1;
        }
        if ((freq - FREQ_TABLE[low - 1]) < (FREQ_TABLE[low] - freq)) {
            return low - 1;
        }
        return low;
    }

    private void notifyListener() {
        if (listener != null) {
            listener.soundStateChanged(this, state);
        }
    }
}
