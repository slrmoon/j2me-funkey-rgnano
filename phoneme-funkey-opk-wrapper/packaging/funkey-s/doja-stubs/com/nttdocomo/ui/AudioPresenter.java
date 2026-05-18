package com.nttdocomo.ui;

public class AudioPresenter implements MediaPresenter {
    private static final int MAX_PRESENTERS = 4;

    private MediaResource resource;
    private MediaListener listener;
    private int type;
    private int volume = 100;

    public static AudioPresenter getAudioPresenter() {
        return getAudioPresenter(0);
    }

    public static AudioPresenter getAudioPresenter(int type) {
        if (type < 0 || type >= MAX_PRESENTERS) {
            throw new UIException("audio presenter unavailable");
        }
        AudioPresenter presenter = new AudioPresenter();
        presenter.type = type;
        return presenter;
    }

    public void setMediaResource(MediaResource r) {
        if (resource != r && resource instanceof DoJaPlayableSound) {
            ((DoJaPlayableSound) resource).stop();
        }
        resource = r;
    }

    public void setSound(MediaSound sound) {
        setMediaResource(sound);
    }

    public void setMediaListener(MediaListener l) {
        listener = l;
    }

    public void setAttribute(int key, int value) {
        if (key == 4) {
            volume = value;
        }
    }

    public MediaResource getMediaResource() {
        return resource;
    }

    public int getCurrentTime() {
        return 0;
    }

    public void play() {
        if (resource instanceof DoJaPlayableSound) {
            ((DoJaPlayableSound) resource).play(volume);
        }
        if (listener != null) {
            listener.mediaAction(this, 0, 0);
        }
    }

    public void stop() {
        if (resource instanceof DoJaPlayableSound) {
            ((DoJaPlayableSound) resource).stop();
        }
    }
}
