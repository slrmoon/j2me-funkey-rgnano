/*
 * FunKey audio engine: the controls of FkaPlayer.
 *
 *   all content players  VolumeControl, StopTimeControl
 *   sampled audio        + RateControl
 *   MIDI and melodies    + MIDIControl, TempoControl (a RateControl), PitchControl
 *   tone sequences       + ToneControl
 *   device://midi        VolumeControl, MIDIControl
 *   device://tone        VolumeControl, StopTimeControl, ToneControl, Tempo, Pitch
 */
package javax.microedition.media;

import java.util.Vector;

import javax.microedition.media.control.MIDIControl;
import javax.microedition.media.control.PitchControl;
import javax.microedition.media.control.RateControl;
import javax.microedition.media.control.StopTimeControl;
import javax.microedition.media.control.TempoControl;
import javax.microedition.media.control.ToneControl;
import javax.microedition.media.control.VolumeControl;

final class FkaControls {
    private FkaControls() {
    }

    static Control[] create(FkaPlayer p, int origin) {
        Vector v = new Vector(6);
        v.addElement(new Volume(p));
        if (origin == FkaPlayer.TYPE_MIDI_DEVICE) {
            v.addElement(new Midi(p));
        } else {
            v.addElement(new StopTime(p));
            if (p.kind() == FkaNative.KIND_PCM) {
                v.addElement(new Rate(p));
            } else if (p.kind() == FkaNative.KIND_SONG) {
                v.addElement(new Tempo(p));
                v.addElement(new Pitch(p));
                if (origin == FkaPlayer.TYPE_CONTENT) v.addElement(new Midi(p));
                if (p.isToneSequence()) v.addElement(new Tone(p));
            }
        }
        Control[] c = new Control[v.size()];
        v.copyInto(c);
        return c;
    }

    static boolean implementsType(Control c, String type) {
        if (type.equals("javax.microedition.media.control.VolumeControl")) return c instanceof VolumeControl;
        if (type.equals("javax.microedition.media.control.StopTimeControl")) return c instanceof StopTimeControl;
        if (type.equals("javax.microedition.media.control.RateControl")) return c instanceof RateControl;
        if (type.equals("javax.microedition.media.control.TempoControl")) return c instanceof TempoControl;
        if (type.equals("javax.microedition.media.control.PitchControl")) return c instanceof PitchControl;
        if (type.equals("javax.microedition.media.control.MIDIControl")) return c instanceof MIDIControl;
        if (type.equals("javax.microedition.media.control.ToneControl")) return c instanceof ToneControl;
        return false;
    }

    /* ---------------------------------------------------------------- */

    static final class Volume implements VolumeControl {
        private final FkaPlayer p;

        Volume(FkaPlayer p) {
            this.p = p;
        }

        public void setMute(boolean mute) {
            boolean changed;
            synchronized (p) {
                changed = p.muted != mute;
                p.muted = mute;
                p.applyVolume();
            }
            if (changed) p.postVolumeChanged(this);
        }

        public boolean isMuted() {
            return p.muted;
        }

        public int setLevel(int level) {
            boolean changed;
            if (level < 0) level = 0;
            if (level > 100) level = 100;
            synchronized (p) {
                changed = p.level != level;
                p.level = level;
                p.applyVolume();
            }
            if (changed) p.postVolumeChanged(this);
            return level;
        }

        public int getLevel() {
            return p.level;
        }
    }

    static final class StopTime implements StopTimeControl {
        private final FkaPlayer p;

        StopTime(FkaPlayer p) {
            this.p = p;
        }

        public void setStopTime(long stopTime) {
            p.setStopTime(stopTime);
        }

        public long getStopTime() {
            return p.getStopTime();
        }
    }

    static class Rate implements RateControl {
        final FkaPlayer p;

        Rate(FkaPlayer p) {
            this.p = p;
        }

        public int setRate(int millirate) {
            if (millirate < getMinRate()) millirate = getMinRate();
            if (millirate > getMaxRate()) millirate = getMaxRate();
            synchronized (p) {
                p.rate = FkaNative.nSetRate(p.nativeId(), millirate);
                return p.rate;
            }
        }

        public int getRate() {
            return p.rate;
        }

        public int getMaxRate() {
            return 400000;
        }

        public int getMinRate() {
            return 10000;
        }
    }

    static final class Tempo extends Rate implements TempoControl {
        Tempo(FkaPlayer p) {
            super(p);
        }

        public int setTempo(int millitempo) {
            if (millitempo < 10000) millitempo = 10000;
            if (millitempo > 300000) millitempo = 300000;
            synchronized (p) {
                /* tempo is the song tempo scaled by the playback rate */
                int t = FkaNative.nSetTempo(p.nativeId(), millitempo);
                p.rate = FkaNative.nGetRate(p.nativeId());
                return t;
            }
        }

        public int getTempo() {
            return FkaNative.nGetTempo(p.nativeId());
        }
    }

    static final class Pitch implements PitchControl {
        private final FkaPlayer p;

        Pitch(FkaPlayer p) {
            this.p = p;
        }

        public int setPitch(int millisemitones) {
            if (millisemitones < getMinPitch()) millisemitones = getMinPitch();
            if (millisemitones > getMaxPitch()) millisemitones = getMaxPitch();
            /* the synth transposes in whole semitones */
            int semis = (millisemitones >= 0 ? millisemitones + 500 : millisemitones - 500) / 1000;
            synchronized (p) {
                p.pitch = semis * 1000;
                FkaNative.nSetPitch(p.nativeId(), p.pitch);
                return p.pitch;
            }
        }

        public int getPitch() {
            return p.pitch;
        }

        public int getMaxPitch() {
            return 24000;
        }

        public int getMinPitch() {
            return -24000;
        }
    }

    static final class Tone implements ToneControl {
        private final FkaPlayer p;

        Tone(FkaPlayer p) {
            this.p = p;
        }

        public void setSequence(byte[] sequence) {
            if (sequence == null) throw new IllegalArgumentException("null sequence");
            synchronized (p) {
                if (p.rawState() == Player.STARTED) throw new IllegalStateException("Player is started");
                if (FkaNative.nReplaceToneSequence(p.nativeId(), sequence) < 0) {
                    throw new IllegalArgumentException("Bad tone sequence");
                }
            }
        }
    }

    static final class Midi implements MIDIControl {
        private final FkaPlayer p;

        Midi(FkaPlayer p) {
            this.p = p;
        }

        private static void checkChannel(int ch) {
            if (ch < 0 || ch > 15) throw new IllegalArgumentException("Bad channel " + ch);
        }

        public boolean isBankQuerySupported() {
            return false;
        }

        public int[] getProgram(int channel) throws MediaException {
            checkChannel(channel);
            int prog = FkaNative.nMidiProgram(p.nativeId(), channel);
            int bank = FkaNative.nMidiBank(p.nativeId(), channel);
            if (prog < 0) prog = 0;
            if (bank < 0) bank = 0;
            return new int[] {bank, prog};
        }

        public int getChannelVolume(int channel) {
            checkChannel(channel);
            return FkaNative.nMidiChannelVolume(p.nativeId(), channel);
        }

        public void setProgram(int channel, int bank, int program) {
            checkChannel(channel);
            if (program < 0 || program > 127 || bank < -1 || bank > 16383) {
                throw new IllegalArgumentException("Bad bank/program");
            }
            if (bank >= 0) {
                FkaNative.nMidiShort(p.nativeId(), 0xB0 | channel, 0, bank >> 7);
                FkaNative.nMidiShort(p.nativeId(), 0xB0 | channel, 32, bank & 0x7F);
            }
            FkaNative.nMidiShort(p.nativeId(), 0xC0 | channel, program, 0);
        }

        public void setChannelVolume(int channel, int volume) {
            checkChannel(channel);
            if (volume < 0 || volume > 127) throw new IllegalArgumentException("Bad volume");
            FkaNative.nMidiShort(p.nativeId(), 0xB0 | channel, 7, volume);
        }

        public int[] getBankList(boolean custom) throws MediaException {
            throw new MediaException("Bank query not supported");
        }

        public int[] getProgramList(int bank) throws MediaException {
            throw new MediaException("Bank query not supported");
        }

        public String getProgramName(int bank, int prog) throws MediaException {
            throw new MediaException("Bank query not supported");
        }

        public String getKeyName(int bank, int prog, int key) throws MediaException {
            throw new MediaException("Bank query not supported");
        }

        public void shortMidiEvent(int type, int data1, int data2) {
            if (type < 0x80 || type > 0xFF || type == 0xF0 || type == 0xF7
                    || data1 < 0 || data1 > 127 || data2 < 0 || data2 > 127) {
                throw new IllegalArgumentException("Bad MIDI event");
            }
            if (type >= 0xF0) return;   /* system messages: nothing to do */
            FkaNative.nMidiShort(p.nativeId(), type, data1, data2);
        }

        public int longMidiEvent(byte[] data, int offset, int length) {
            if (data == null || offset < 0 || length < 0 || offset + length > data.length) {
                throw new IllegalArgumentException("Bad MIDI data");
            }
            if (length == 0) return 0;
            /* a run of short messages or one sysex */
            if ((data[offset] & 0xFF) == 0xF0) {
                FkaNative.nMidiLong(p.nativeId(), data, offset, length);
                return length;
            }
            int i = offset, end = offset + length, status = 0;
            while (i < end) {
                int b = data[i] & 0xFF;
                if (b >= 0x80) {
                    status = b;
                    i++;
                }
                if (status < 0x80 || status >= 0xF0) return i - offset;
                int hi = status & 0xF0;
                int n = (hi == 0xC0 || hi == 0xD0) ? 1 : 2;
                if (i + n > end) break;
                FkaNative.nMidiShort(p.nativeId(), status, data[i] & 0x7F, n == 2 ? data[i + 1] & 0x7F : 0);
                i += n;
            }
            return i - offset;
        }
    }
}
