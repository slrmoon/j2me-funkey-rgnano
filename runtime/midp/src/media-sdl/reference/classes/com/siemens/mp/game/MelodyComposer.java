/*
 * Siemens melody composer: notes C0..A4 (A1 = 440 Hz), note lengths from a
 * whole note to 1/64, dotted lengths, pauses and repeat marks.
 */
package com.siemens.mp.game;

import javax.microedition.media.control.ToneControl;

public class MelodyComposer extends com.siemens.mp.misc.NativeMem {
    public static final int TONE_C0 = 0;
    public static final int TONE_CIS0 = 1;
    public static final int TONE_D0 = 2;
    public static final int TONE_DIS0 = 3;
    public static final int TONE_E0 = 4;
    public static final int TONE_F0 = 5;
    public static final int TONE_FIS0 = 6;
    public static final int TONE_G0 = 7;
    public static final int TONE_GIS0 = 8;
    public static final int TONE_A0 = 9;
    public static final int TONE_AIS0 = 10;
    public static final int TONE_H0 = 11;
    public static final int TONE_C1 = 12;
    public static final int TONE_CIS1 = 13;
    public static final int TONE_D1 = 14;
    public static final int TONE_DIS1 = 15;
    public static final int TONE_E1 = 16;
    public static final int TONE_F1 = 17;
    public static final int TONE_FIS1 = 18;
    public static final int TONE_G1 = 19;
    public static final int TONE_GIS1 = 20;
    public static final int TONE_A1 = 21;
    public static final int TONE_AIS1 = 22;
    public static final int TONE_H1 = 23;
    public static final int TONE_C2 = 24;
    public static final int TONE_CIS2 = 25;
    public static final int TONE_D2 = 26;
    public static final int TONE_DIS2 = 27;
    public static final int TONE_E2 = 28;
    public static final int TONE_F2 = 29;
    public static final int TONE_FIS2 = 30;
    public static final int TONE_G2 = 31;
    public static final int TONE_GIS2 = 32;
    public static final int TONE_A2 = 33;
    public static final int TONE_AIS2 = 34;
    public static final int TONE_H2 = 35;
    public static final int TONE_C3 = 36;
    public static final int TONE_CIS3 = 37;
    public static final int TONE_D3 = 38;
    public static final int TONE_DIS3 = 39;
    public static final int TONE_E3 = 40;
    public static final int TONE_F3 = 41;
    public static final int TONE_FIS3 = 42;
    public static final int TONE_G3 = 43;
    public static final int TONE_GIS3 = 44;
    public static final int TONE_A3 = 45;
    public static final int TONE_AIS3 = 46;
    public static final int TONE_H3 = 47;
    public static final int TONE_C4 = 48;
    public static final int TONE_CIS4 = 49;
    public static final int TONE_D4 = 50;
    public static final int TONE_DIS4 = 51;
    public static final int TONE_E4 = 52;
    public static final int TONE_F4 = 53;
    public static final int TONE_FIS4 = 54;
    public static final int TONE_G4 = 55;
    public static final int TONE_GIS4 = 56;
    public static final int TONE_A4 = 57;
    public static final int TONE_PAUSE = 58;
    public static final int NO_TONE = 59;
    public static final int TONE_STOP = 60;
    public static final int TONE_REPEAT = 61;
    public static final int TONE_REPEV = 62;
    public static final int TONE_REPON = 63;
    public static final int TONE_MARK = 64;
    public static final int TONE_REPEAT_MARK = 65;
    public static final int TONE_REPEV_MARK = 66;
    public static final int TONE_REPON_MARK = 67;

    public static final int TONELENGTH_1_1 = 0;
    public static final int TONELENGTH_1_2 = 1;
    public static final int TONELENGTH_1_4 = 2;
    public static final int TONELENGTH_1_8 = 3;
    public static final int TONELENGTH_1_16 = 4;
    public static final int TONELENGTH_1_32 = 5;
    public static final int TONELENGTH_1_64 = 6;
    public static final int TONELENGTH_DOTTED_1_1 = 7;
    public static final int TONELENGTH_DOTTED_1_2 = 8;
    public static final int TONELENGTH_DOTTED_1_4 = 9;
    public static final int TONELENGTH_DOTTED_1_8 = 10;
    public static final int TONELENGTH_DOTTED_1_16 = 11;
    public static final int TONELENGTH_DOTTED_1_32 = 12;
    public static final int TONELENGTH_DOTTED_1_64 = 13;

    public static final int BPM = 60;

    private static final int MAX_NOTES = 1024;
    private static final int MAX_UNROLLED = 4096;
    private static final int C0_MIDI = 48;        /* A1 = 440 Hz */

    private int[] notes = new int[64];
    private int[] lengths = new int[64];
    private int count;
    private int bpm = BPM;

    public MelodyComposer() {
    }

    /** notes: pairs of (note, length). */
    public MelodyComposer(int[] notes, int bpm) {
        setBPM(bpm);
        if (notes != null) {
            for (int i = 0; i + 1 < notes.length; i += 2) appendNote(notes[i], notes[i + 1]);
        }
    }

    public void appendNote(int note, int length) {
        if (count >= MAX_NOTES) throw new IllegalArgumentException("Melody too long");
        if (count == notes.length) {
            int[] n = new int[count * 2], l = new int[count * 2];
            System.arraycopy(notes, 0, n, 0, count);
            System.arraycopy(lengths, 0, l, 0, count);
            notes = n;
            lengths = l;
        }
        notes[count] = note;
        lengths[count] = length;
        count++;
    }

    public Melody getMelody() {
        return build();
    }

    public int length() {
        return count;
    }

    public static int maxLength() {
        return MAX_NOTES;
    }

    public void resetMelody() {
        count = 0;
    }

    public void setBPM(int bpm) {
        if (bpm < 20) bpm = 20;
        if (bpm > 508) bpm = 508;
        this.bpm = bpm;
    }

    /* units of 1/64 whole note */
    private static int units(int length) {
        switch (length) {
            case TONELENGTH_1_1: return 64;
            case TONELENGTH_1_2: return 32;
            case TONELENGTH_1_4: return 16;
            case TONELENGTH_1_8: return 8;
            case TONELENGTH_1_16: return 4;
            case TONELENGTH_1_32: return 2;
            case TONELENGTH_1_64: return 1;
            case TONELENGTH_DOTTED_1_1: return 96;
            case TONELENGTH_DOTTED_1_2: return 48;
            case TONELENGTH_DOTTED_1_4: return 24;
            case TONELENGTH_DOTTED_1_8: return 12;
            case TONELENGTH_DOTTED_1_16: return 6;
            case TONELENGTH_DOTTED_1_32: return 3;
            default: return 16;
        }
    }

    private Melody build() {
        /* unroll repeats into a flat list of (note, units) */
        int[] outN = new int[64], outU = new int[64];
        int n = 0, mark = 0;
        boolean forever = false;
        int[] repeatsLeft = new int[count];
        for (int i = 0; i < count; i++) repeatsLeft[i] = -1;
        for (int i = 0; i < count && n < MAX_UNROLLED; i++) {
            int note = notes[i];
            if (note == TONE_STOP) break;
            if (note == TONE_MARK) {
                mark = n;
                continue;
            }
            if (note == TONE_REPEV || note == TONE_REPON) {
                forever = true;   /* loop the whole melody */
                break;
            }
            if (note == TONE_REPEV_MARK || note == TONE_REPON_MARK) {
                /* loop from the mark: play the tail a few times */
                int len = n - mark;
                for (int k = 0; k < 8 && len > 0 && n + len < MAX_UNROLLED; k++) {
                    if (n + len > outN.length) {
                        outN = grow(outN, n + len);
                        outU = grow(outU, n + len);
                    }
                    System.arraycopy(outN, mark, outN, n, len);
                    System.arraycopy(outU, mark, outU, n, len);
                    n += len;
                }
                break;
            }
            if (note == TONE_REPEAT || note == TONE_REPEAT_MARK) {
                int from = note == TONE_REPEAT ? 0 : mark;
                int times = lengths[i] > 0 ? lengths[i] : 1;
                int len = n - from;
                for (int k = 0; k < times && len > 0 && n + len < MAX_UNROLLED; k++) {
                    if (n + len > outN.length) {
                        outN = grow(outN, n + len);
                        outU = grow(outU, n + len);
                    }
                    System.arraycopy(outN, from, outN, n, len);
                    System.arraycopy(outU, from, outU, n, len);
                    n += len;
                }
                continue;
            }
            if (note == NO_TONE) continue;
            if (n == outN.length) {
                outN = grow(outN, n + 1);
                outU = grow(outU, n + 1);
            }
            outN[n] = note == TONE_PAUSE || note < 0 || note > TONE_A4 ? ToneControl.SILENCE : C0_MIDI + note;
            outU[n] = units(lengths[i]);
            n++;
        }
        if (n == 0) return new Melody(null, false);
        byte[] seq = new byte[6 + 2 * n];
        seq[0] = ToneControl.VERSION;
        seq[1] = 1;
        seq[2] = ToneControl.TEMPO;
        seq[3] = (byte) (bpm / 4);
        seq[4] = ToneControl.RESOLUTION;
        seq[5] = 64;
        for (int i = 0; i < n; i++) {
            seq[6 + 2 * i] = (byte) outN[i];
            seq[7 + 2 * i] = (byte) outU[i];
        }
        return new Melody(seq, forever);
    }

    private static int[] grow(int[] a, int min) {
        int[] b = new int[Math.max(min, a.length * 2)];
        System.arraycopy(a, 0, b, 0, a.length);
        return b;
    }

    static int freqToNote(int hz) {
        double f = 440.0;
        int note = 69;
        while (hz > f * 1.0293 && note < 127) {
            f *= 1.0594630943592953;
            note++;
        }
        while (hz < f / 1.0293 && note > 0) {
            f /= 1.0594630943592953;
            note--;
        }
        return note;
    }
}
