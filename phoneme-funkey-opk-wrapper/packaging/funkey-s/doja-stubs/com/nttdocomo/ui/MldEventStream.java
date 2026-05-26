package com.nttdocomo.ui;

import java.io.ByteArrayOutputStream;

final class MldEventStream {
    private static final int[] TIME_BASE_TABLE = {
        6, 12, 24, 48, 96, 192, 384, -1,
        15, 30, 60, 120, 240, 480, 960, -1
    };

    private static final int EVENT_PROGRAM = 1;
    private static final int EVENT_VOLUME = 2;
    private static final int EVENT_PAN = 3;
    private static final int EVENT_NOTE_ON = 4;
    private static final int EVENT_NOTE_OFF = 5;
    private static final int EVENT_LOOP_START = 6;
    private static final int EVENT_LOOP_END = 7;
    private static final int EVENT_END = 8;
    private static final int EVENT_PITCH = 9;
    private static final int EVENT_MODULATION = 10;
    private static final int EVENT_PITCH_RANGE = 11;
    private static final int EVENT_EXPRESSION = 12;

    private MldEventStream() {
    }

    static byte[] build(byte[] mld, boolean dump) {
        if (mld == null || mld.length < 16 || !hasTag(mld, 0, "melo")) {
            return null;
        }

        int noteLength = readSmallChunkByte(mld, "note", 1);
        if (noteLength <= 0) {
            noteLength = 1;
        }

        EventList events = new EventList();
        int tracks = 0;
        int notes = 0;
        int maxTime = 0;
        boolean[] drumChannels = new boolean[16];
        for (int pos = 8; pos + 8 <= mld.length && tracks < 4; pos++) {
            if (!hasTag(mld, pos, "trac")) {
                continue;
            }
            int len = read32(mld, pos + 4);
            int start = pos + 8;
            if (len <= 0 || start + len > mld.length) {
                continue;
            }
            TrackResult result = parseTrack(mld, start, len, noteLength, tracks,
                    drumChannels, events, dump);
            notes += result.notes;
            if (result.maxTimeMs > maxTime) {
                maxTime = result.maxTimeMs;
            }
            tracks++;
            pos = start + len - 1;
        }

        if (tracks <= 0 || events.size <= 0) {
            return null;
        }
        events.add(maxTime + 1, EVENT_END, 0, 0, 0);
        events.sort();

        ByteArrayOutputStream out = new ByteArrayOutputStream(events.size * 8 + 8);
        out.write('D');
        out.write('J');
        out.write('A');
        out.write('1');
        write16(out, events.size);
        write16(out, 22050);
        for (int i = 0; i < events.size; i++) {
            Event e = events.items[i];
            write32(out, e.timeMs);
            out.write(e.type);
            out.write(e.channel);
            out.write(e.a);
            out.write(e.b);
            if (dump) {
                dumpEvent(e);
            }
        }
        byte[] stream = out.toByteArray();
        if (isDebugEnabled()) {
            System.out.println("DoJa MLD event stream tracks=" + tracks
                    + " notes=" + notes + " events=" + events.size
                    + " bytes=" + stream.length);
        }
        return stream;
    }

    private static TrackResult parseTrack(byte[] data, int start, int len, int noteLength,
            int trackIndex, boolean[] drumChannels, EventList events, boolean dump) {
        int pos = start;
        int end = start + len;
        int timeMs = 0;
        int maxTimeMs = 0;
        int notes = 0;
        TrackState state = new TrackState(drumChannels);
        while (pos + 2 <= end) {
            int delta = data[pos++] & 255;
            int status = data[pos++] & 255;
            timeMs += ticksToMs(delta, state.tempo);
            if (status == 0x3f || status == 0x7f || status == 0xbf || status == 0xff) {
                if (pos >= end) {
                    break;
                }
                int data1 = data[pos++] & 255;
                int next = parseControl(data, pos, end, data1, state, events, timeMs,
                        trackIndex, dump);
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
            int channel = eventChannel(state, trackIndex, voice);
            int vel = velocity * 2;
            if (vel > 127) {
                vel = 127;
            }
            int offMs = timeMs + ticksToMs(gate, state.tempo);
            if (offMs <= timeMs) {
                offMs = timeMs + 1;
            }
            events.add(timeMs, EVENT_NOTE_ON, channel, midi, vel);
            events.add(offMs, EVENT_NOTE_OFF, channel, midi, 0);
            if (offMs > maxTimeMs) {
                maxTimeMs = offMs;
            }
            notes++;
        }
        return new TrackResult(notes, maxTimeMs);
    }

    private static int parseControl(byte[] data, int pos, int end, int data1, TrackState state,
            EventList events, int timeMs, int trackIndex, boolean dump) {
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
            applyShortControl(data1, data2, state, events, timeMs, trackIndex);
            return pos + 1;
        }
        if (pos + 2 > end) {
            return -1;
        }
        int len = read16(data, pos);
        if (pos + 2 + len > end) {
            return -1;
        }
        if (dump) {
            System.out.println("DoJa MLD sysex time=" + timeMs
                    + " data1=" + data1 + " len=" + len);
        }
        return pos + 2 + len;
    }

    private static void applyShortControl(int data1, int data2, TrackState state,
            EventList events, int timeMs, int trackIndex) {
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
            for (int i = 0; i < 4; i++) {
                int channel = eventChannel(state, trackIndex, i);
                events.add(timeMs, EVENT_VOLUME, channel,
                        state.volume[i] * state.masterVolume / 63, 0);
            }
            return;
        }
        if (data1 == 0xba) {
            int pseudoChannel = (data2 & 0x78) >> 3;
            if (pseudoChannel >= 0 && pseudoChannel < state.drumChannels.length) {
                state.drumChannels[pseudoChannel] = (data2 & 0x07) == 1;
                if (state.drumChannels[pseudoChannel]) {
                    events.add(timeMs, EVENT_PROGRAM, 9, 0, 0);
                }
            }
            return;
        }

        int voice = (data2 & 0xc0) >> 6;
        int value = data2 & 0x3f;
        int channel = eventChannel(state, trackIndex, voice);
        switch (data1) {
        case 0xe0:
            state.program[voice] = (state.program[voice] & 0x40) | value;
            events.add(timeMs, EVENT_PROGRAM, channel, state.program[voice], state.bank[voice]);
            break;
        case 0xe1:
            state.bank[voice] = value;
            state.program[voice] = ((value & 1) << 6) | (state.program[voice] & 0x3f);
            break;
        case 0xe2:
            state.volume[voice] = value;
            events.add(timeMs, EVENT_VOLUME, channel, value * state.masterVolume / 63, 0);
            break;
        case 0xe3:
            state.pan[voice] = value;
            events.add(timeMs, EVENT_PAN, channel, value * 2, 0);
            break;
        case 0xe4:
            state.pitch[voice] = value;
            events.add(timeMs, EVENT_PITCH, channel, value, 0);
            break;
        case 0xe6:
            state.expression[voice] = (value & 0x20) != 0 ? value - 64 : value;
            events.add(timeMs, EVENT_EXPRESSION, channel,
                    clamp(64 + state.expression[voice] * 2), 0);
            break;
        case 0xe7:
            state.pitchRange[voice] = value;
            events.add(timeMs, EVENT_PITCH_RANGE, channel, value, 0);
            break;
        case 0xea:
            state.modulation[voice] = value;
            events.add(timeMs, EVENT_MODULATION, channel, value, 0);
            break;
        default:
            break;
        }
    }

    private static void dumpEvent(Event e) {
        String name;
        if (e.type == EVENT_PROGRAM) {
            name = "program=" + e.a;
        } else if (e.type == EVENT_VOLUME) {
            name = "volume=" + e.a;
        } else if (e.type == EVENT_PAN) {
            name = "pan=" + e.a;
        } else if (e.type == EVENT_NOTE_ON) {
            name = "note_on " + e.a + " vel=" + e.b;
        } else if (e.type == EVENT_NOTE_OFF) {
            name = "note_off " + e.a;
        } else if (e.type == EVENT_LOOP_START) {
            name = "loop_start";
        } else if (e.type == EVENT_LOOP_END) {
            name = "loop_end count=" + e.a;
        } else if (e.type == EVENT_PITCH) {
            name = "pitch=" + e.a;
        } else if (e.type == EVENT_MODULATION) {
            name = "modulation=" + e.a;
        } else if (e.type == EVENT_PITCH_RANGE) {
            name = "pitch_range=" + e.a;
        } else if (e.type == EVENT_EXPRESSION) {
            name = "expression=" + e.a;
        } else {
            name = "end";
        }
        System.out.println("DoJa MLD event time=" + e.timeMs + " ch=" + e.channel + " " + name);
    }

    private static int eventChannel(TrackState state, int trackIndex, int voice) {
        int pseudo = (trackIndex * 4 + (voice & 3)) & 15;
        if (state.drumChannels[pseudo]) {
            return 9;
        }
        return pseudo == 9 ? 15 : pseudo;
    }

    private static int clamp(int value) {
        if (value < 0) {
            return 0;
        }
        return value > 127 ? 127 : value;
    }

    private static boolean isDebugEnabled() {
        return "true".equals(System.getProperty("doja.audio.debug"))
                || System.getProperty("doja.audio.dump") != null;
    }

    private static int ticksToMs(int ticks, int tempo) {
        if (ticks <= 0) {
            return 0;
        }
        if (tempo < 20) {
            tempo = 20;
        }
        int ms = (int) (ticks * 60000L / (48L * (long) tempo));
        return ms < 1 ? 1 : ms;
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

    private static boolean hasTag(byte[] data, int pos, String tag) {
        return pos >= 0 && pos + tag.length() <= data.length
                && data[pos] == (byte) tag.charAt(0)
                && data[pos + 1] == (byte) tag.charAt(1)
                && data[pos + 2] == (byte) tag.charAt(2)
                && data[pos + 3] == (byte) tag.charAt(3);
    }

    private static int read16(byte[] data, int pos) {
        return ((data[pos] & 255) << 8) | (data[pos + 1] & 255);
    }

    private static int read32(byte[] data, int pos) {
        return ((data[pos] & 255) << 24) | ((data[pos + 1] & 255) << 16)
                | ((data[pos + 2] & 255) << 8) | (data[pos + 3] & 255);
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

    private static final class TrackState {
        int timeBase = 48;
        int tempo = 125;
        int masterVolume = 127;
        int[] program = new int[4];
        int[] bank = new int[4];
        int[] volume = { 63, 63, 63, 63 };
        int[] pan = { 32, 32, 32, 32 };
        int[] pitch = { 32, 32, 32, 32 };
        int[] pitchRange = { 2, 2, 2, 2 };
        int[] modulation = { 0, 0, 0, 0 };
        int[] expression = { 64, 64, 64, 64 };
        boolean[] drumChannels;

        TrackState(boolean[] drums) {
            drumChannels = drums;
        }
    }

    private static final class TrackResult {
        int notes;
        int maxTimeMs;

        TrackResult(int n, int t) {
            notes = n;
            maxTimeMs = t;
        }
    }

    private static final class Event {
        int timeMs;
        int type;
        int channel;
        int a;
        int b;
    }

    private static final class EventList {
        Event[] items = new Event[64];
        int size;

        void add(int timeMs, int type, int channel, int a, int b) {
            if (size == items.length) {
                Event[] next = new Event[items.length * 2];
                System.arraycopy(items, 0, next, 0, items.length);
                items = next;
            }
            Event e = new Event();
            e.timeMs = timeMs;
            e.type = type;
            e.channel = channel & 15;
            e.a = a & 255;
            e.b = b & 255;
            items[size++] = e;
        }

        void sort() {
            for (int i = 1; i < size; i++) {
                Event e = items[i];
                int j = i - 1;
                while (j >= 0 && compare(items[j], e) > 0) {
                    items[j + 1] = items[j];
                    j--;
                }
                items[j + 1] = e;
            }
        }

        private int compare(Event a, Event b) {
            if (a.timeMs != b.timeMs) {
                return a.timeMs - b.timeMs;
            }
            return priority(a.type) - priority(b.type);
        }

        private int priority(int type) {
            switch (type) {
            case EVENT_PROGRAM:
            case EVENT_VOLUME:
            case EVENT_PAN:
            case EVENT_PITCH:
            case EVENT_MODULATION:
            case EVENT_PITCH_RANGE:
            case EVENT_EXPRESSION:
                return 1;
            case EVENT_NOTE_OFF:
                return 2;
            case EVENT_NOTE_ON:
                return 3;
            case EVENT_LOOP_START:
            case EVENT_LOOP_END:
                return 4;
            case EVENT_END:
                return 5;
            default:
                return 6;
            }
        }
    }
}
