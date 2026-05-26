package com.nttdocomo.ui;

import java.io.ByteArrayOutputStream;

final class MldPcmDecoder {
    private static final int SYNTH_RATE = 11025;
    private static final int SYNTH_MAX_MS = 30000;
    private static final int[] TIME_BASE_TABLE = {
        6, 12, 24, 48, 96, 192, 384, -1,
        15, 30, 60, 120, 240, 480, 960, -1
    };

    private static final int[] YM2608_STEP_TABLE = {
        57, 57, 57, 57, 77, 102, 128, 153,
        57, 57, 57, 57, 77, 102, 128, 153
    };

    private MldPcmDecoder() {
    }

    static byte[] decodeToMidi(byte[] mld) {
        if (mld == null || mld.length < 16 || !hasTag(mld, 0, "melo")) {
            return null;
        }
        int noteLength = readSmallChunkByte(mld, "note", 1);
        if (noteLength <= 0) {
            noteLength = 1;
        }

        ByteArrayOutputStream out = new ByteArrayOutputStream();
        writeAscii(out, "MThd");
        write32(out, 6);
        write16(out, 1);

        int trackCount = countTracks(mld);
        if (trackCount <= 0) {
            return null;
        }
        if (trackCount > 4) {
            trackCount = 4;
        }
        write16(out, trackCount);
        write16(out, 96);

        int trackIndex = 0;
        int totalNotes = 0;
        boolean[] drumChannels = new boolean[16];
        for (int pos = 8; pos + 8 <= mld.length && trackIndex < trackCount; pos++) {
            if (!hasTag(mld, pos, "trac")) {
                continue;
            }
            int len = read32(mld, pos + 4);
            int start = pos + 8;
            if (len <= 0 || start + len > mld.length) {
                continue;
            }
            MidiTrackResult result = buildMidiTrack(mld, start, len, noteLength, trackIndex, drumChannels);
            if (result != null) {
                writeAscii(out, "MTrk");
                write32(out, result.data.length);
                out.write(result.data, 0, result.data.length);
                totalNotes += result.notes;
                trackIndex++;
            }
            pos = start + len - 1;
        }
        if (trackIndex <= 0 || totalNotes <= 0) {
            return null;
        }
        byte[] midi = out.toByteArray();
        System.out.println("DoJa MLD converted to MIDI v9 tracks=" + trackIndex
                + " notes=" + totalNotes + " bytes=" + midi.length);
        return midi;
    }

    static byte[] decodeToWav(byte[] mld) {
        if (mld == null || mld.length < 16 || !hasTag(mld, 0, "melo")) {
            return null;
        }
        for (int pos = 8; pos + 8 <= mld.length;) {
            int len = read32(mld, pos + 4);
            int dataStart = pos + 8;
            int next = dataStart + len;
            if (len < 0 || next > mld.length) {
                pos++;
                continue;
            }
            if (hasTag(mld, pos, "adat")) {
                byte[] wav = decodeAudioData(mld, dataStart, len);
                if (wav != null) {
                    return wav;
                }
            }
            pos = next;
        }
        return renderTracksToWav(mld);
    }

    private static int countTracks(byte[] mld) {
        int tracks = 0;
        for (int pos = 8; pos + 8 <= mld.length; pos++) {
            if (!hasTag(mld, pos, "trac")) {
                continue;
            }
            int len = read32(mld, pos + 4);
            int start = pos + 8;
            if (len > 0 && start + len <= mld.length) {
                tracks++;
                pos = start + len - 1;
            }
        }
        return tracks;
    }

    private static MidiTrackResult buildMidiTrack(byte[] data, int start, int len, int noteLength,
            int trackIndex, boolean[] drumChannels) {
        int pos = start;
        int end = start + len;
        int tick = 0;
        int notes = 0;
        TrackState state = new TrackState(drumChannels);
        MidiEventList events = new MidiEventList();
        while (pos + 2 <= end) {
            int delta = data[pos++] & 255;
            int status = data[pos++] & 255;
            tick += scaleTicks(delta, state.timeBase);
            if (status == 0x3f || status == 0x7f || status == 0xbf || status == 0xff) {
                if (pos >= end) {
                    break;
                }
                int data1 = data[pos++] & 255;
                int next = appendMidiControl(data, pos, end, data1, state, events, tick, trackIndex);
                if (next < 0) {
                    break;
                }
                pos = next;
                continue;
            }
            if (pos >= end) {
                break;
            }
            int gate = data[pos++] & 255;
            int velocity = 63;
            int shift = 0;
            if (noteLength == 1 && pos < end) {
                int data2 = data[pos++] & 255;
                velocity = (data2 & 0xfc) >> 2;
                shift = data2 & 3;
            }
            int note = status & 0x3f;
            if (shift == 1) {
                note += 12;
            } else if (shift == 2) {
                note -= 24;
            } else if (shift == 3) {
                note -= 12;
            }
            int voice = (status >> 6) & 3;
            int pseudoChannel = (trackIndex * 4 + voice) & 15;
            int midi = note + (state.drumChannels[pseudoChannel] ? 35 : 45);
            if (midi < 0) {
                midi = 0;
            } else if (midi > 127) {
                midi = 127;
            }
            int channel = midiChannel(state, trackIndex, voice);
            int vel = velocity * 2;
            if (vel > 127) {
                vel = 127;
            }
            int offTick = tick + scaleTicks(gate, state.timeBase);
            if (offTick <= tick) {
                offTick = tick + 1;
            }
            events.add(tick, 10, new byte[] { (byte) (0x90 | channel), (byte) midi, (byte) vel });
            events.add(offTick, 0, new byte[] { (byte) (0x80 | channel), (byte) midi, 0 });
            notes++;
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        int lastTick = 0;
        for (int i = 0; i < events.size; i++) {
            MidiEvent event = events.items[i];
            writeVar(out, event.tick - lastTick);
            out.write(event.data, 0, event.data.length);
            lastTick = event.tick;
        }
        writeVar(out, 0);
        out.write(0xff);
        out.write(0x2f);
        out.write(0);
        return new MidiTrackResult(out.toByteArray(), notes);
    }

    private static int appendMidiControl(byte[] data, int pos, int end, int data1, TrackState state,
            MidiEventList events, int tick, int trackIndex) {
        if (data1 >= 0x00 && data1 <= 0x7f) {
            if (pos + 2 > end) {
                return -1;
            }
            return pos + 2;
        }
        if (data1 >= 0x80 && data1 <= 0xef) {
            if (pos >= end) {
                return -1;
            }
            int data2 = data[pos] & 255;
            appendShortMidiControl(data1, data2, state, events, tick, trackIndex);
            return pos + 1;
        }
        if (pos + 2 > end) {
            return -1;
        }
        int sysexLen = read16(data, pos);
        if (pos + 2 + sysexLen > end) {
            return -1;
        }
        return pos + 2 + sysexLen;
    }

    private static void appendShortMidiControl(int data1, int data2, TrackState state,
            MidiEventList events, int tick, int trackIndex) {
        if ((data1 & 0xf0) == 0xc0) {
            int base = TIME_BASE_TABLE[data1 & 15];
            if (base > 0) {
                state.timeBase = base;
            }
            state.tempo = data2 < 20 ? 20 : data2;
            int micros = 60000000 / state.tempo;
            events.add(tick, 1, new byte[] { (byte) 0xff, 0x51, 0x03,
                    (byte) ((micros >> 16) & 255),
                    (byte) ((micros >> 8) & 255),
                    (byte) (micros & 255) });
            return;
        }
        if (data1 == 0xb0) {
            state.masterVolume = data2 > 127 ? 127 : data2;
            for (int i = 0; i < 4; i++) {
                int channel = midiChannel(state, trackIndex, i);
                appendController(events, tick, channel, 7, state.volume[i] * state.masterVolume / 63);
            }
            return;
        }
        if (data1 == 0xba) {
            int pseudoChannel = (data2 & 0x78) >> 3;
            if (pseudoChannel >= 0 && pseudoChannel < state.drumChannels.length) {
                state.drumChannels[pseudoChannel] = (data2 & 0x07) == 1;
                if (state.drumChannels[pseudoChannel]) {
                    events.add(tick, 2, new byte[] { (byte) 0xc9, 0 });
                }
            }
            return;
        }
        int voice = (data2 & 0xc0) >> 6;
        int value = data2 & 0x3f;
        int channel = midiChannel(state, trackIndex, voice);
        switch (data1) {
        case 0xe0:
            state.program[voice] = (state.program[voice] & 0x40) | value;
            events.add(tick, 3, new byte[] { (byte) (0xc0 | channel), (byte) (state.program[voice] & 0x7f) });
            break;
        case 0xe1:
            state.bank[voice] = value;
            state.program[voice] = ((value & 1) << 6) | (state.program[voice] & 0x3f);
            break;
        case 0xe2:
            state.volume[voice] = value;
            appendController(events, tick, channel, 7, value * state.masterVolume / 63);
            break;
        case 0xe3:
            state.pan[voice] = value;
            appendController(events, tick, channel, 10, value * 2);
            break;
        case 0xe4:
            state.pitchBend[voice] = value - 32;
            int bend = 8192 + state.pitchBend[voice] * 256;
            if (bend < 0) {
                bend = 0;
            } else if (bend > 16383) {
                bend = 16383;
            }
            events.add(tick, 4, new byte[] { (byte) (0xe0 | channel), (byte) (bend & 127), (byte) ((bend >> 7) & 127) });
            break;
        case 0xe7:
            appendController(events, tick, channel, 100, 0);
            appendController(events, tick, channel, 101, 0);
            appendController(events, tick, channel, 6, value > 24 ? 24 : value);
            break;
        case 0xe6:
            state.expression[voice] = (value & 0x20) != 0 ? value - 64 : value;
            appendController(events, tick, channel, 11, 64 + state.expression[voice] * 2);
            break;
        case 0xea:
            state.modulation[voice] = value;
            appendController(events, tick, channel, 1, value * 2);
            break;
        default:
            break;
        }
    }

    private static int midiChannel(TrackState state, int trackIndex, int voice) {
        int pseudo = (trackIndex * 4 + (voice & 3)) & 15;
        if (state.drumChannels[pseudo]) {
            return 9;
        }
        return pseudo == 9 ? 15 : pseudo;
    }

    private static void appendController(MidiEventList events, int tick, int channel, int control, int value) {
        if (value < 0) {
            value = 0;
        } else if (value > 127) {
            value = 127;
        }
        events.add(tick, 2, new byte[] { (byte) (0xb0 | channel), (byte) control, (byte) value });
    }

    private static int scaleTicks(int ticks, int timeBase) {
        int scaled = ticks * 2;
        return ticks > 0 && scaled <= 0 ? 1 : scaled;
    }

    private static byte[] renderTracksToWav(byte[] mld) {
        int noteLength = readSmallChunkByte(mld, "note", 1);
        if (noteLength <= 0) {
            noteLength = 1;
        }
        short[] pcm = new short[SYNTH_RATE / 2];
        int maxSample = 0;
        int tracks = 0;
        int notes = 0;
        for (int pos = 8; pos + 8 <= mld.length; pos++) {
            if (!hasTag(mld, pos, "trac")) {
                continue;
            }
            int len = read32(mld, pos + 4);
            int start = pos + 8;
            if (len <= 0 || start + len > mld.length) {
                continue;
            }
            RenderResult result = renderTrack(mld, start, len, noteLength, tracks, pcm, maxSample);
            pcm = result.pcm;
            tracks++;
            notes += result.notes;
            if (result.maxSample > maxSample) {
                maxSample = result.maxSample;
            }
            pos = start + len - 1;
        }
        if (maxSample <= 0) {
            return null;
        }
        int tail = SYNTH_RATE / 20;
        if (maxSample + tail < pcm.length) {
            maxSample += tail;
        }
        byte[] bytes = new byte[maxSample * 2];
        for (int i = 0; i < maxSample; i++) {
            int v = pcm[i];
            bytes[i * 2] = (byte) (v & 255);
            bytes[i * 2 + 1] = (byte) ((v >> 8) & 255);
        }
        System.out.println("DoJa MLD rendered v6 tracks=" + tracks + " notes=" + notes
                + " pcmBytes=" + bytes.length);
        return makeWav(bytes, SYNTH_RATE, 1);
    }

    private static RenderResult renderTrack(byte[] data, int start, int len, int noteLength, int trackIndex,
            short[] pcm, int initialMaxSample) {
        int pos = start;
        int end = start + len;
        TrackState state = new TrackState();
        int sampleCursor = 0;
        int maxSample = initialMaxSample;
        int notes = 0;
        while (pos + 2 <= end) {
            int delta = data[pos++] & 255;
            int status = data[pos++] & 255;
            sampleCursor += ticksToSamples(delta, state.tempo);
            if (status == 0x3f || status == 0x7f || status == 0xbf || status == 0xff) {
                if (pos >= end) {
                    break;
                }
                int data1 = data[pos++] & 255;
                int next = applyControlMessage(data, pos, end, data1, state);
                if (next < 0) {
                    break;
                }
                pos = next;
                continue;
            }
            if (pos >= end) {
                break;
            }
            int gate = data[pos++] & 255;
            int velocity = 63;
            int shift = 0;
            if (noteLength == 1 && pos < end) {
                int data2 = data[pos++] & 255;
                velocity = (data2 & 0xfc) >> 2;
                shift = data2 & 3;
            }
            int note = status & 0x3f;
            if (shift == 1) {
                note += 12;
            } else if (shift == 2) {
                note -= 24;
            } else if (shift == 3) {
                note -= 12;
            }
            int voice = status >> 6;
            int pseudoChannel = (trackIndex * 4 + voice) & 15;
            int midi = note + (state.drumChannels[pseudoChannel] ? 35 : 45);
            int startSample = sampleCursor;
            int sampleLen = ticksToSamples(gate, state.tempo);
            int maxCapacity = maxSynthSamples();
            if (startSample >= maxCapacity) {
                break;
            }
            if (sampleLen < SYNTH_RATE / 60) {
                sampleLen = SYNTH_RATE / 60;
            }
            int needed = startSample + sampleLen + SYNTH_RATE / 20;
            if (needed < startSample || needed > maxCapacity) {
                needed = maxCapacity;
                sampleLen = needed - startSample;
            }
            if (needed > pcm.length) {
                pcm = grow(pcm, needed);
            }
            mixNote(pcm, startSample, sampleLen, midi, velocity, voice, state);
            notes++;
            int renderedEnd = needed < pcm.length ? needed : pcm.length;
            if (renderedEnd > maxSample) {
                maxSample = renderedEnd;
            }
        }
        return new RenderResult(pcm, maxSample, notes);
    }

    private static int applyControlMessage(byte[] data, int pos, int end, int data1, TrackState state) {
        if (data1 >= 0x00 && data1 <= 0x7f) {
            if (pos + 2 > end) {
                return -1;
            }
            return pos + 2;
        }
        if (data1 >= 0x80 && data1 <= 0xef) {
            if (pos >= end) {
                return -1;
            }
            int data2 = data[pos] & 255;
            applyShortControl(data1, data2, state);
            return pos + 1;
        }
        if (pos + 2 > end) {
            return -1;
        }
        int len = read16(data, pos);
        if (pos + 2 + len > end) {
            return -1;
        }
        return pos + 2 + len;
    }

    private static void applyShortControl(int data1, int data2, TrackState state) {
        if ((data1 & 0xf0) == 0xc0) {
            int base = TIME_BASE_TABLE[data1 & 15];
            if (base > 0) {
                state.timeBase = base;
            }
            state.tempo = data2 < 20 ? 20 : data2;
            return;
        }
        if (data1 == 0xb0) {
            state.masterVolume = data2 > 127 ? 127 : data2;
            return;
        }
        int voice = (data2 & 0xc0) >> 6;
        int value = data2 & 0x3f;
        switch (data1) {
        case 0xe0:
            state.program[voice] = (state.program[voice] & 0x40) | value;
            break;
        case 0xe1:
            state.bank[voice] = value;
            state.program[voice] = ((value & 1) << 6) | (state.program[voice] & 0x3f);
            break;
        case 0xe2:
            state.volume[voice] = value;
            break;
        case 0xe3:
            state.pan[voice] = value;
            break;
        case 0xe4:
            state.pitchBend[voice] = value - 32;
            break;
        case 0xe6:
            state.expression[voice] = (value & 0x20) != 0 ? value - 64 : value;
            break;
        case 0xea:
            state.modulation[voice] = value;
            break;
        default:
            break;
        }
    }

    private static void mixNote(short[] pcm, int start, int samples, int midi, int velocity, int voice,
            TrackState state) {
        if (midi < 0 || midi > 127 || start >= pcm.length) {
            return;
        }
        int program = state.program[voice & 3];
        double freq = midiToFreq(midi) * bendToRatio(state.pitchBend[voice & 3]);
        double phase = (voice & 3) * 0.17;
        double step = freq / (double) SYNTH_RATE;
        int volume = 120 + velocity * 42;
        volume = volume * state.volume[voice & 3] / 63;
        volume = volume * (96 + state.expression[voice & 3]) / 128;
        volume = volume * state.masterVolume / 127;
        int attack = attackSamples(program);
        int release = releaseSamples(program);
        int modulation = state.modulation[voice & 3];
        for (int i = 0; i < samples && start + i < pcm.length; i++) {
            double env = 1.0;
            if (i < attack) {
                env = (double) i / (double) attack;
            } else if (i > samples - release) {
                env = (double) (samples - i) / (double) release;
            }
            if (env < 0.0) {
                env = 0.0;
            }
            double wave = waveform(phase, program, i, modulation);
            int sample = (int) (wave * volume * env);
            int mixed = pcm[start + i] + sample;
            if (mixed > 32767) {
                mixed = 32767;
            } else if (mixed < -32768) {
                mixed = -32768;
            }
            pcm[start + i] = (short) mixed;
            phase += step;
            phase -= (int) phase;
        }
    }

    private static short[] grow(short[] pcm, int needed) {
        int capacity = pcm.length;
        int max = maxSynthSamples();
        while (capacity < needed && capacity < max) {
            if (capacity > max / 2) {
                capacity = max;
                break;
            }
            capacity *= 2;
        }
        if (capacity < needed) {
            capacity = needed;
        }
        if (capacity > max) {
            capacity = max;
        }
        short[] next = new short[capacity];
        System.arraycopy(pcm, 0, next, 0, pcm.length);
        return next;
    }

    private static double midiToFreq(int midi) {
        double freq = 440.0;
        int diff = midi - 69;
        while (diff > 0) {
            freq *= 1.0594630943592953;
            diff--;
        }
        while (diff < 0) {
            freq /= 1.0594630943592953;
            diff++;
        }
        return freq;
    }

    private static int ticksToSamples(int ticks) {
        return ticksToSamples(ticks, 125);
    }

    private static int ticksToSamples(int ticks, int tempo) {
        if (ticks <= 0) {
            return 0;
        }
        if (tempo < 20) {
            tempo = 20;
        }
        double seconds = ticks * 60.0 / (48.0 * (double) tempo);
        int samples = (int) (seconds * SYNTH_RATE + 0.5);
        return samples < 1 ? 1 : samples;
    }

    private static int maxSynthSamples() {
        return SYNTH_RATE * (SYNTH_MAX_MS / 1000);
    }

    private static double bendToRatio(int bend) {
        if (bend == 0) {
            return 1.0;
        }
        double ratio = 1.0;
        double step = 1.0036166659754628; // two semitones over roughly 32 MFi bend steps
        int n = bend < 0 ? -bend : bend;
        while (n-- > 0) {
            ratio = bend > 0 ? ratio * step : ratio / step;
        }
        return ratio;
    }

    private static int attackSamples(int program) {
        int group = program & 7;
        if (group == 0 || group == 1) {
            return SYNTH_RATE / 400;
        }
        if (group == 2 || group == 3) {
            return SYNTH_RATE / 120;
        }
        return SYNTH_RATE / 80;
    }

    private static int releaseSamples(int program) {
        int group = program & 7;
        if (group == 0 || group == 6) {
            return SYNTH_RATE / 50;
        }
        if (group == 2 || group == 3) {
            return SYNTH_RATE / 30;
        }
        return SYNTH_RATE / 80;
    }

    private static double waveform(double phase, int program, int sampleIndex, int modulation) {
        int family = program & 7;
        double p = phase;
        if (modulation > 0) {
            p += ((sampleIndex / (20 + (modulation & 31))) & 1) == 0 ? 0.003 * modulation : -0.003 * modulation;
            p -= (int) p;
        }
        if (family == 0) {
            return p < 0.5 ? 0.70 : -0.70;
        }
        if (family == 1) {
            return p < 0.5 ? p * 4.0 - 1.0 : 3.0 - p * 4.0;
        }
        if (family == 2) {
            return 1.0 - p * 2.0;
        }
        if (family == 3) {
            double tri = p < 0.5 ? p * 4.0 - 1.0 : 3.0 - p * 4.0;
            return tri * 0.75 + (p < 0.25 || (p > 0.5 && p < 0.75) ? 0.20 : -0.20);
        }
        if (family == 4) {
            return p < 0.125 ? 0.85 : -0.30;
        }
        if (family == 5) {
            double tri = p < 0.5 ? p * 4.0 - 1.0 : 3.0 - p * 4.0;
            return tri * (0.55 + 0.25 * (1.0 - p));
        }
        if (family == 6) {
            int noise = ((sampleIndex * 1103515245 + program * 12345) >> 16) & 255;
            return (noise - 128) / 160.0;
        }
        return p < 0.5 ? p * 3.2 - 0.8 : 2.4 - p * 3.2;
    }

    private static int readSmallChunkByte(byte[] data, String tag, int index) {
        for (int pos = 8; pos + 6 <= data.length; pos++) {
            if (hasTag(data, pos, tag)) {
                int len = read16(data, pos + 4);
                int value = pos + 6 + index;
                if (len > index && value < data.length) {
                    return data[value] & 255;
                }
            }
        }
        return -1;
    }

    private static byte[] decodeAudioData(byte[] data, int start, int len) {
        if (len < 6 || start + len > data.length) {
            return null;
        }
        int headerLen = read16(data, start);
        int format = data[start + 2] & 255;
        int subStart = start + 4;
        int subEnd = subStart + headerLen - 2;
        if (headerLen < 2 || subEnd > start + len) {
            return null;
        }

        int sampleRate = 8000;
        int bits = 4;
        int channels = 1;
        boolean interleaved = false;
        for (int p = subStart; p + 6 <= subEnd;) {
            int subLen = read16(data, p + 4);
            int payload = p + 6;
            if (payload + subLen > subEnd) {
                break;
            }
            if (hasTag(data, p, "adpm") && subLen >= 3) {
                sampleRate = (data[payload] & 255) * 1000;
                bits = data[payload + 1] & 255;
                interleaved = (data[payload + 2] & 8) != 0;
                channels = data[payload + 2] & 7;
            }
            p = payload + subLen;
        }

        if (format != 0x81 || bits != 4 || sampleRate <= 0 || (channels != 1 && channels != 2)) {
            return null;
        }

        int adpcmStart = subEnd;
        int adpcmLen = len - (adpcmStart - start);
        if (adpcmLen <= 0 || adpcmStart + adpcmLen > data.length) {
            return null;
        }

        byte[] pcm = channels == 2
                ? decodeStereo(data, adpcmStart, adpcmLen, interleaved)
                : decodeMono(data, adpcmStart, adpcmLen);
        if (pcm == null || pcm.length == 0) {
            return null;
        }
        return makeWav(pcm, sampleRate, channels);
    }

    private static byte[] decodeMono(byte[] data, int off, int len) {
        AdpcmState state = new AdpcmState();
        ByteArrayOutputStream out = new ByteArrayOutputStream(len * 4);
        for (int i = 0; i < len; i++) {
            int b = data[off + i] & 255;
            write16le(out, state.decode((b >> 4) & 15));
            write16le(out, state.decode(b & 15));
        }
        return out.toByteArray();
    }

    private static byte[] decodeStereo(byte[] data, int off, int len, boolean interleaved) {
        AdpcmState left = new AdpcmState();
        AdpcmState right = new AdpcmState();
        ByteArrayOutputStream out = new ByteArrayOutputStream(len * 4);
        if (interleaved) {
            boolean useLeft = true;
            for (int i = 0; i < len; i++) {
                int b = data[off + i] & 255;
                useLeft = decodeStereoNibble(out, left, right, (b >> 4) & 15, useLeft);
                useLeft = decodeStereoNibble(out, left, right, b & 15, useLeft);
            }
        } else {
            int half = len / 2;
            int leftSamples = half * 2;
            short[] l = decodeChannel(data, off, half);
            short[] r = decodeChannel(data, off + half, len - half);
            int samples = l.length < r.length ? l.length : r.length;
            if (samples > leftSamples) {
                samples = leftSamples;
            }
            for (int i = 0; i < samples; i++) {
                write16le(out, l[i]);
                write16le(out, r[i]);
            }
        }
        return out.toByteArray();
    }

    private static boolean decodeStereoNibble(ByteArrayOutputStream out, AdpcmState left,
            AdpcmState right, int nibble, boolean useLeft) {
        if (useLeft) {
            write16le(out, left.decode(nibble));
        } else {
            write16le(out, right.decode(nibble));
        }
        return !useLeft;
    }

    private static short[] decodeChannel(byte[] data, int off, int len) {
        AdpcmState state = new AdpcmState();
        short[] pcm = new short[len * 2];
        int p = 0;
        for (int i = 0; i < len; i++) {
            int b = data[off + i] & 255;
            pcm[p++] = (short) state.decode((b >> 4) & 15);
            pcm[p++] = (short) state.decode(b & 15);
        }
        return pcm;
    }

    private static byte[] makeWav(byte[] pcm, int sampleRate, int channels) {
        ByteArrayOutputStream out = new ByteArrayOutputStream(44 + pcm.length);
        writeAscii(out, "RIFF");
        write32le(out, 36 + pcm.length);
        writeAscii(out, "WAVEfmt ");
        write32le(out, 16);
        write16le(out, 1);
        write16le(out, channels);
        write32le(out, sampleRate);
        write32le(out, sampleRate * channels * 2);
        write16le(out, channels * 2);
        write16le(out, 16);
        writeAscii(out, "data");
        write32le(out, pcm.length);
        out.write(pcm, 0, pcm.length);
        return out.toByteArray();
    }

    private static boolean hasTag(byte[] data, int off, String tag) {
        return off >= 0 && off + 4 <= data.length
                && data[off] == (byte) tag.charAt(0)
                && data[off + 1] == (byte) tag.charAt(1)
                && data[off + 2] == (byte) tag.charAt(2)
                && data[off + 3] == (byte) tag.charAt(3);
    }

    private static int read16(byte[] data, int off) {
        return ((data[off] & 255) << 8) | (data[off + 1] & 255);
    }

    private static int read32(byte[] data, int off) {
        return ((data[off] & 255) << 24) | ((data[off + 1] & 255) << 16)
                | ((data[off + 2] & 255) << 8) | (data[off + 3] & 255);
    }

    private static void writeAscii(ByteArrayOutputStream out, String text) {
        for (int i = 0; i < text.length(); i++) {
            out.write((byte) text.charAt(i));
        }
    }

    private static void write16le(ByteArrayOutputStream out, int value) {
        out.write(value & 255);
        out.write((value >> 8) & 255);
    }

    private static void write16(ByteArrayOutputStream out, int value) {
        out.write((value >> 8) & 255);
        out.write(value & 255);
    }

    private static void write32(ByteArrayOutputStream out, int value) {
        out.write((value >> 24) & 255);
        out.write((value >> 16) & 255);
        out.write((value >> 8) & 255);
        out.write(value & 255);
    }

    private static void write32le(ByteArrayOutputStream out, int value) {
        out.write(value & 255);
        out.write((value >> 8) & 255);
        out.write((value >> 16) & 255);
        out.write((value >> 24) & 255);
    }

    private static void writeVar(ByteArrayOutputStream out, int value) {
        if (value < 0) {
            value = 0;
        }
        int buffer = value & 0x7f;
        while ((value >>= 7) > 0) {
            buffer <<= 8;
            buffer |= ((value & 0x7f) | 0x80);
        }
        for (;;) {
            out.write(buffer & 255);
            if ((buffer & 0x80) != 0) {
                buffer >>= 8;
            } else {
                break;
            }
        }
    }

    private static final class AdpcmState {
        private int stepSize = 127;
        private int sample;

        int decode(int adpcm) {
            int delta = ((adpcm & 7) * 2 + 1) * stepSize / 8;
            if ((adpcm & 8) != 0) {
                sample -= delta;
            } else {
                sample += delta;
            }
            if (sample > 32767) {
                sample = 32767;
            } else if (sample < -32768) {
                sample = -32768;
            }
            stepSize = stepSize * YM2608_STEP_TABLE[adpcm & 15] / 64;
            if (stepSize < 127) {
                stepSize = 127;
            } else if (stepSize > 24576) {
                stepSize = 24576;
            }
            return sample;
        }
    }

    private static final class RenderResult {
        final short[] pcm;
        final int maxSample;
        final int notes;

        RenderResult(short[] pcm, int maxSample, int notes) {
            this.pcm = pcm;
            this.maxSample = maxSample;
            this.notes = notes;
        }
    }

    private static final class MidiTrackResult {
        final byte[] data;
        final int notes;

        MidiTrackResult(byte[] data, int notes) {
            this.data = data;
            this.notes = notes;
        }
    }

    private static final class MidiEvent {
        final int tick;
        final int order;
        final byte[] data;

        MidiEvent(int tick, int order, byte[] data) {
            this.tick = tick;
            this.order = order;
            this.data = data;
        }
    }

    private static final class MidiEventList {
        MidiEvent[] items = new MidiEvent[64];
        int size;

        void add(int tick, int order, byte[] data) {
            MidiEvent event = new MidiEvent(tick, order, data);
            if (size >= items.length) {
                MidiEvent[] next = new MidiEvent[items.length * 2];
                System.arraycopy(items, 0, next, 0, items.length);
                items = next;
            }
            int at = size;
            while (at > 0 && compare(event, items[at - 1]) < 0) {
                items[at] = items[at - 1];
                at--;
            }
            items[at] = event;
            size++;
        }

        private int compare(MidiEvent a, MidiEvent b) {
            if (a.tick != b.tick) {
                return a.tick < b.tick ? -1 : 1;
            }
            if (a.order != b.order) {
                return a.order < b.order ? -1 : 1;
            }
            return 0;
        }
    }

    private static final class TrackState {
        final boolean[] drumChannels;
        int timeBase = 48;
        int tempo = 125;
        int masterVolume = 127;
        final int[] bank = { 0, 0, 0, 0 };
        final int[] program = { 0, 1, 2, 3 };
        final int[] volume = { 63, 63, 63, 63 };
        final int[] pan = { 32, 32, 32, 32 };
        final int[] pitchBend = { 0, 0, 0, 0 };
        final int[] expression = { 0, 0, 0, 0 };
        final int[] modulation = { 0, 0, 0, 0 };

        TrackState() {
            this(new boolean[16]);
        }

        TrackState(boolean[] drums) {
            drumChannels = drums == null ? new boolean[16] : drums;
        }
    }
}
