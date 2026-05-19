package com.nttdocomo.ui;

import java.io.ByteArrayOutputStream;
import java.io.ByteArrayInputStream;
import java.io.InputStream;

import javax.microedition.io.Connector;
import javax.microedition.media.Manager;
import javax.microedition.media.Player;
import javax.microedition.media.control.VolumeControl;

import com.funkey.doja.ResourceLoader;

public final class MediaManager {
    private MediaManager() {
    }

    public static MediaImage getImage(String name) {
        return new SimpleMediaImage(name);
    }

    public static MediaImage getImage(byte[] data) {
        return new SimpleMediaImage(data);
    }

    public static MediaSound getSound(String name) {
        return new SimpleMediaSound(name);
    }

    private static final class SimpleMediaImage implements MediaImage {
        private String name;
        private byte[] data;
        private Image image;

        SimpleMediaImage(String n) {
            name = n;
        }

        SimpleMediaImage(byte[] bytes) {
            data = bytes;
        }

        public void use() {
            try {
                image = data == null ? Image.createImage(name) : Image.createImage(data);
            } catch (Throwable t) {
                t.printStackTrace();
            }
        }

        public void unuse() {
            image = null;
        }

        public void dispose() {
            if (image != null) {
                image.dispose();
            }
            image = null;
            data = null;
            name = null;
        }

        public Image getImage() {
            if (image == null) {
                use();
            }
            return image;
        }
    }

    private static final class SimpleMediaSound implements MediaSound, DoJaPlayableSound {
        private static final SimpleMediaSound[] activeSounds = new SimpleMediaSound[8];

        private String name;
        private byte[] data;
        private byte[] cachedMidi;
        private byte[] cachedWav;
        private Player player;
        private DoJaAudioPlayer dojaPlayer;
        private boolean loaded;
        private int note;
        private boolean playing;

        SimpleMediaSound(String n) {
            name = n;
            note = 48 + Math.abs(n == null ? 0 : n.hashCode()) % 36;
        }

        public void use() {
            if (loaded) {
                return;
            }
            loaded = true;
            try {
                data = readSoundData(name);
                System.out.println("DoJa sound loaded v8 name=" + name
                        + " bytes=" + (data == null ? 0 : data.length)
                        + " type=" + guessType(name, data));
            } catch (Throwable t) {
                System.out.println("DoJa sound load failed name=" + name + " error=" + t);
            }
        }

        public void unuse() {
            closeDoJaPlayer();
            closePlayer();
            cachedMidi = null;
            cachedWav = null;
            data = null;
            loaded = false;
        }

        public void dispose() {
            unuse();
            name = null;
        }

        public void play(int volume) {
            use();
            if (volume <= 0) {
                return;
            }
            stopMatchingActiveSound(this);
            if (isMld(name, data) && isDoJaStreamingEnabled() && playDoJaAudio(volume)) {
                return;
            }
            byte[] midi = cachedMidi;
            if (midi == null) {
                midi = MldPcmDecoder.decodeToMidi(data);
                if (midi != null && midi.length <= 65536) {
                    cachedMidi = midi;
                }
            }
            if (midi != null && playMidi(midi, volume)) {
                return;
            }
            byte[] wav = cachedWav;
            if (wav == null) {
                wav = MldPcmDecoder.decodeToWav(data);
                if (wav != null && wav.length <= 131072) {
                    cachedWav = wav;
                }
            }
            if (wav != null) {
                playPcm(wav, volume);
                return;
            }
            if (playing) {
                return;
            }
            final int scaledVolume = volume > 100 ? 100 : volume;
            final int[] notes = extractMeloNotes(data, note);
            playing = true;
            new Thread(new Runnable() {
                public void run() {
                    try {
                        for (int i = 0; playing && i < notes.length; i++) {
                            Manager.playTone(notes[i], 70, scaledVolume);
                            Thread.sleep(75);
                        }
                    } catch (Throwable t) {
                        System.out.println("DoJa sound play failed name=" + name + " error=" + t);
                    } finally {
                        playing = false;
                    }
                }
            }).start();
        }

        public void stop() {
            playing = false;
            unregisterActiveSound(this);
            closeDoJaPlayer();
            closePlayer();
        }

        private boolean playDoJaAudio(int volume) {
            try {
                closePlayer();
                if (dojaPlayer == null) {
                    dojaPlayer = new DoJaAudioPlayer(data, name);
                    dojaPlayer.realize();
                    dojaPlayer.prefetch();
                }
                dojaPlayer.setVolume(volume > 100 ? 100 : volume);
                dojaPlayer.start();
                registerActiveSound(this);
                return true;
            } catch (Throwable t) {
                System.out.println("DoJa streaming sound failed name=" + name + " error=" + t);
                closeDoJaPlayer();
                return false;
            }
        }

        private static boolean isDoJaStreamingEnabled() {
            return "true".equals(System.getProperty("doja.audio.streaming"));
        }

        private boolean playMidi(byte[] midi, int volume) {
            try {
                closePlayer();
                System.out.println("DoJa MIDI sound play name=" + name + " bytes=" + midi.length);
                player = Manager.createPlayer(new ByteArrayInputStream(midi), "audio/midi");
                player.realize();
                Object control = player.getControl("javax.microedition.media.control.VolumeControl");
                if (control instanceof VolumeControl) {
                    ((VolumeControl) control).setLevel(volume > 100 ? 100 : volume);
                }
                player.prefetch();
                player.start();
                registerActiveSound(this);
                return true;
            } catch (Throwable t) {
                System.out.println("DoJa MIDI sound play failed name=" + name + " error=" + t);
                closePlayer();
                return false;
            }
        }

        private void playPcm(byte[] wav, int volume) {
            try {
                closePlayer();
                System.out.println("DoJa PCM sound play name=" + name + " bytes=" + wav.length);
                player = Manager.createPlayer(new ByteArrayInputStream(wav), "audio/x-wav");
                player.realize();
                Object control = player.getControl("javax.microedition.media.control.VolumeControl");
                if (control instanceof VolumeControl) {
                    ((VolumeControl) control).setLevel(volume > 100 ? 100 : volume);
                }
                player.prefetch();
                player.start();
                registerActiveSound(this);
            } catch (Throwable t) {
                System.out.println("DoJa PCM sound play failed name=" + name + " error=" + t);
                closePlayer();
            }
        }

        private void closePlayer() {
            Player old = player;
            player = null;
            if (old != null) {
                try {
                    old.stop();
                } catch (Throwable ignored) {
                }
                try {
                    old.close();
                } catch (Throwable ignored) {
                }
            }
        }

        private void closeDoJaPlayer() {
            DoJaAudioPlayer old = dojaPlayer;
            dojaPlayer = null;
            if (old != null) {
                try {
                    old.stop();
                } catch (Throwable ignored) {
                }
                try {
                    old.close();
                } catch (Throwable ignored) {
                }
            }
        }

        private static void stopMatchingActiveSound(SimpleMediaSound sound) {
            synchronized (activeSounds) {
                for (int i = 0; i < activeSounds.length; i++) {
                    SimpleMediaSound active = activeSounds[i];
                    if (active != null && active != sound && sameSound(active.name, sound.name)) {
                        active.playing = false;
                        active.closeDoJaPlayer();
                        active.closePlayer();
                        activeSounds[i] = null;
                    }
                }
            }
        }

        private static void registerActiveSound(SimpleMediaSound sound) {
            synchronized (activeSounds) {
                unregisterActiveSoundLocked(sound);
                for (int i = 0; i < activeSounds.length; i++) {
                    if (activeSounds[i] == null) {
                        activeSounds[i] = sound;
                        return;
                    }
                }
                SimpleMediaSound old = activeSounds[0];
                if (old != null && old != sound) {
                    old.playing = false;
                    old.closeDoJaPlayer();
                    old.closePlayer();
                }
                activeSounds[0] = sound;
            }
        }

        private static void unregisterActiveSound(SimpleMediaSound sound) {
            synchronized (activeSounds) {
                unregisterActiveSoundLocked(sound);
            }
        }

        private static void unregisterActiveSoundLocked(SimpleMediaSound sound) {
            for (int i = 0; i < activeSounds.length; i++) {
                if (activeSounds[i] == sound) {
                    activeSounds[i] = null;
                }
            }
        }

        private static boolean sameSound(String a, String b) {
            return a == b || (a != null && a.equals(b));
        }
    }

    private static byte[] readSoundData(String name) throws Exception {
        InputStream in;
        if (name != null && (name.startsWith("scratchpad://")
                || name.startsWith("resource://"))) {
            in = Connector.openInputStream(name);
        } else {
            in = new ResourceLoader().open(name);
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] head = new byte[8];
        int got = readFully(in, head, 0, head.length);
        if (got > 0) {
            out.write(head, 0, got);
        }
        int limit;
        if (got == 8 && head[0] == 'm' && head[1] == 'e'
                && head[2] == 'l' && head[3] == 'o') {
            limit = (((head[4] & 255) << 24) | ((head[5] & 255) << 16)
                    | ((head[6] & 255) << 8) | (head[7] & 255));
        } else if (name != null && name.indexOf(";length=") >= 0) {
            limit = Integer.MAX_VALUE;
        } else {
            limit = 4096 - got;
        }

        byte[] buf = new byte[512];
        int total = 0;
        while (total < limit) {
            int want = buf.length;
            if (limit != Integer.MAX_VALUE && total + want > limit) {
                want = limit - total;
            }
            int n = in.read(buf, 0, want);
            if (n < 0) {
                break;
            }
            out.write(buf, 0, n);
            total += n;
        }
        in.close();
        return out.toByteArray();
    }

    private static int readFully(InputStream in, byte[] buf, int off, int len) throws Exception {
        int total = 0;
        while (total < len) {
            int n = in.read(buf, off + total, len - total);
            if (n < 0) {
                break;
            }
            total += n;
        }
        return total;
    }

    private static String guessType(String name, byte[] data) {
        String lower = name == null ? "" : name.toLowerCase();
        if (lower.endsWith(".mld")) {
            return "audio/mld";
        }
        if (lower.endsWith(".mid") || lower.endsWith(".midi")) {
            return "audio/midi";
        }
        if (lower.endsWith(".wav")) {
            return "audio/x-wav";
        }
        if (data != null && data.length >= 4) {
            if (data[0] == 'm' && data[1] == 'e' && data[2] == 'l' && data[3] == 'o') {
                return "audio/mld";
            }
            if (data[0] == 'M' && data[1] == 'T' && data[2] == 'h' && data[3] == 'd') {
                return "audio/midi";
            }
            if (data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F') {
                return "audio/x-wav";
            }
        }
        return "unknown";
    }

    private static boolean isMld(String name, byte[] data) {
        return "audio/mld".equals(guessType(name, data));
    }

    private static int[] extractMeloNotes(byte[] data, int fallbackNote) {
        int[] notes = new int[8];
        int count = 0;
        if (data != null && data.length > 16
                && data[0] == 'm' && data[1] == 'e' && data[2] == 'l' && data[3] == 'o') {
            int start = findChunk(data, "trac");
            if (start < 0) {
                start = findChunk(data, "adat");
            }
            if (start >= 0) {
                int end = data.length;
                if (start + 8 <= data.length) {
                    int len = (((data[start + 4] & 255) << 24)
                            | ((data[start + 5] & 255) << 16)
                            | ((data[start + 6] & 255) << 8)
                            | (data[start + 7] & 255));
                    start += 8;
                    if (len > 0 && start + len <= data.length) {
                        end = start + len;
                    }
                }
                for (int i = start; i < end && count < notes.length; i++) {
                    int value = data[i] & 255;
                    if (value >= 0x30 && value <= 0x7f
                            && (i == start || (data[i - 1] & 255) != 0xff)) {
                        notes[count++] = 48 + (value % 36);
                        i += 2;
                    }
                }
            }
        }
        if (count == 0) {
            notes[count++] = fallbackNote;
        }
        int[] trimmed = new int[count];
        System.arraycopy(notes, 0, trimmed, 0, count);
        return trimmed;
    }

    private static int findChunk(byte[] data, String name) {
        byte a = (byte) name.charAt(0);
        byte b = (byte) name.charAt(1);
        byte c = (byte) name.charAt(2);
        byte d = (byte) name.charAt(3);
        for (int i = 8; i + 4 <= data.length; i++) {
            if (data[i] == a && data[i + 1] == b
                    && data[i + 2] == c && data[i + 3] == d) {
                return i;
            }
        }
        return -1;
    }
}
