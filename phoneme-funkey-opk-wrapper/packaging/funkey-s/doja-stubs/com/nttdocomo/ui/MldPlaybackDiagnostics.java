package com.nttdocomo.ui;

final class MldPlaybackDiagnostics {
    private static final int STREAM_PROGRAM = 1;
    private static final int STREAM_VOLUME = 2;
    private static final int STREAM_PAN = 3;
    private static final int STREAM_NOTE_ON = 4;
    private static final int STREAM_NOTE_OFF = 5;
    private static final int STREAM_END = 8;
    private static final int STREAM_PITCH = 9;
    private static final int STREAM_MODULATION = 10;
    private static final int STREAM_PITCH_RANGE = 11;
    private static final int STREAM_EXPRESSION = 12;
    private static final int DEFAULT_MAX_ROWS_PER_SOURCE = 80;
    private static final int DEFAULT_MAX_DIFFS = 120;
    private static final String[] comparedNames = new String[64];
    private static int comparedNameCount;

    private MldPlaybackDiagnostics() {
    }

    static boolean isEnabled() {
        String value = System.getProperty("doja.audio.compare");
        return "true".equals(value) || "1".equals(value) || "yes".equals(value);
    }

    static void dumpComparison(byte[] mld, String name) {
        if (!isEnabled()) {
            return;
        }
        if (wasCompared(name)) {
            System.out.println("MLD_COMPARE_SKIPPED duplicate name=" + name);
            return;
        }
        rememberCompared(name);
        try {
            byte[] stream = MldEventStream.build(mld, false);
            byte[] midi = MldPcmDecoder.decodeToMidi(mld);
            RowList streamRows = parseStream(stream);
            RowList legacyRows = parseMidi(midi);
            computeDurations(streamRows);
            computeDurations(legacyRows);
            int maxRows = readLimit("doja.audio.compare.maxRows",
                    DEFAULT_MAX_ROWS_PER_SOURCE);
            int maxDiffs = readLimit("doja.audio.compare.maxDiffs",
                    DEFAULT_MAX_DIFFS);
            System.out.println("MLD_COMPARE_BEGIN name=" + name
                    + " streamEvents=" + streamRows.size
                    + " legacyEvents=" + legacyRows.size
                    + " maxRows=" + maxRows
                    + " maxDiffs=" + maxDiffs);
            System.out.println("MLD_COMPARE_COLUMNS time_ms,source,channel,event_type,note,velocity,program,bank_msb,bank_lsb,cc,value,pitch_bend,duration");
            dumpRows(streamRows, "STREAM", maxRows);
            dumpRows(legacyRows, "LEGACY", maxRows);
            dumpDiff(streamRows, legacyRows, maxDiffs);
            System.out.println("MLD_COMPARE_END name=" + name);
        } catch (Throwable t) {
            System.out.println("MLD_COMPARE_FAILED name=" + name + " error=" + t);
        }
    }

    private static void dumpRows(RowList rows, String source, int maxRows) {
        int limit = rows.size < maxRows ? rows.size : maxRows;
        for (int i = 0; i < limit; i++) {
            Row r = rows.items[i];
            System.out.println("MLD_EVENT time_ms=" + r.timeMs
                    + " source=" + source
                    + " channel=" + value(r.channel)
                    + " event_type=" + r.type
                    + " note=" + value(r.note)
                    + " velocity=" + value(r.velocity)
                    + " program=" + value(r.program)
                    + " bank_msb=" + value(r.bankMsb)
                    + " bank_lsb=" + value(r.bankLsb)
                    + " cc=" + value(r.cc)
                    + " value=" + value(r.value)
                    + " pitch_bend=" + value(r.pitchBend)
                    + " duration=" + value(r.duration));
        }
        if (limit < rows.size) {
            System.out.println("MLD_EVENT_OMITTED source=" + source
                    + " omitted=" + (rows.size - limit));
        }
    }

    private static void dumpDiff(RowList stream, RowList legacy, int maxDiffs) {
        int max = stream.size > legacy.size ? stream.size : legacy.size;
        int mismatches = 0;
        int printed = 0;
        ReasonCounts counts = new ReasonCounts();
        for (int i = 0; i < max; i++) {
            Row s = i < stream.size ? stream.items[i] : null;
            Row l = i < legacy.size ? legacy.items[i] : null;
            if (s == null) {
                printed += printDiffLimited(i, "missing stream event", null, l, printed, maxDiffs);
                counts.add("missing stream event");
                mismatches++;
            } else if (l == null) {
                printed += printDiffLimited(i, "extra stream event", s, null, printed, maxDiffs);
                counts.add("extra stream event");
                mismatches++;
            } else {
                String reason = compareReason(s, l);
                if (reason != null) {
                    printed += printDiffLimited(i, reason, s, l, printed, maxDiffs);
                    counts.add(reason);
                    mismatches++;
                }
            }
        }
        if (printed < mismatches) {
            System.out.println("MLD_DIFF_OMITTED omitted=" + (mismatches - printed));
        }
        counts.print();
        System.out.println("MLD_COMPARE_SUMMARY streamEvents=" + stream.size
                + " legacyEvents=" + legacy.size
                + " mismatches=" + mismatches
                + " printedDiffs=" + printed);
    }

    private static String compareReason(Row s, Row l) {
        if (!sameType(s.type, l.type)) {
            if (isController(s) || isController(l)) {
                return "missing controller";
            }
            return "event type mismatch";
        }
        if (s.channel != l.channel) {
            if (s.channel == 9 || l.channel == 9) {
                return "drum channel mismatch";
            }
            return "channel mismatch";
        }
        if (s.program != l.program) {
            return "different program";
        }
        if (s.bankMsb != l.bankMsb || s.bankLsb != l.bankLsb) {
            return "missing bank";
        }
        if (s.note != l.note) {
            return "note mismatch";
        }
        if (s.velocity != l.velocity) {
            return "velocity mismatch";
        }
        if (s.cc != l.cc || s.value != l.value) {
            return "missing controller";
        }
        if (s.pitchBend != l.pitchBend) {
            return "pitch bend mismatch";
        }
        if ("tempo".equals(s.type) || "tempo".equals(l.type)) {
            if (s.value != l.value) {
                return "tempo mismatch";
            }
        }
        return null;
    }

    private static boolean sameType(String a, String b) {
        if (a == b || (a != null && a.equals(b))) {
            return true;
        }
        if (isControllerName(a) && isControllerName(b)) {
            return true;
        }
        return false;
    }

    private static boolean isController(Row r) {
        return r != null && isControllerName(r.type);
    }

    private static boolean isControllerName(String type) {
        return "control".equals(type) || "volume".equals(type)
                || "pan".equals(type) || "expression".equals(type)
                || "modulation".equals(type) || "pitch_range".equals(type);
    }

    private static void printDiff(int index, String reason, Row stream, Row legacy) {
        System.out.println("MLD_DIFF index=" + index
                + " reason=" + reason
                + " stream=" + brief(stream)
                + " legacy=" + brief(legacy));
    }

    private static int printDiffLimited(int index, String reason, Row stream, Row legacy,
            int printed, int maxDiffs) {
        if (printed < maxDiffs) {
            printDiff(index, reason, stream, legacy);
            return 1;
        }
        return 0;
    }

    private static boolean wasCompared(String name) {
        int count = comparedNameCount < comparedNames.length
                ? comparedNameCount : comparedNames.length;
        for (int i = 0; i < count; i++) {
            if (name == comparedNames[i] || (name != null && name.equals(comparedNames[i]))) {
                return true;
            }
        }
        return false;
    }

    private static void rememberCompared(String name) {
        if (comparedNames.length == 0) {
            return;
        }
        if (comparedNameCount < comparedNames.length) {
            comparedNames[comparedNameCount++] = name;
        } else {
            comparedNames[comparedNameCount % comparedNames.length] = name;
            comparedNameCount++;
        }
    }

    private static int readLimit(String property, int fallback) {
        String value = System.getProperty(property);
        if (value == null) {
            return fallback;
        }
        try {
            int parsed = Integer.parseInt(value);
            if (parsed < 0) {
                return 0;
            }
            if (parsed > 1000) {
                return 1000;
            }
            return parsed;
        } catch (Throwable ignored) {
            return fallback;
        }
    }

    private static String brief(Row r) {
        if (r == null) {
            return "null";
        }
        return r.timeMs + "/" + r.type + "/ch" + value(r.channel)
                + "/note" + value(r.note)
                + "/vel" + value(r.velocity)
                + "/prog" + value(r.program)
                + "/bank" + value(r.bankMsb) + ":" + value(r.bankLsb)
                + "/cc" + value(r.cc)
                + "/val" + value(r.value)
                + "/bend" + value(r.pitchBend)
                + "/dur" + value(r.duration);
    }

    private static RowList parseStream(byte[] stream) {
        RowList rows = new RowList();
        if (stream == null || stream.length < 8 || stream[0] != 'D'
                || stream[1] != 'J' || stream[2] != 'A' || stream[3] != '1') {
            return rows;
        }
        int count = read16(stream, 4);
        int max = 8 + count * 8;
        if (max > stream.length) {
            max = stream.length;
        }
        for (int pos = 8; pos + 8 <= max; pos += 8) {
            int time = read32(stream, pos);
            int type = stream[pos + 4] & 255;
            int channel = stream[pos + 5] & 15;
            int a = stream[pos + 6] & 255;
            int b = stream[pos + 7] & 255;
            Row row = new Row();
            row.timeMs = time;
            row.channel = channel;
            switch (type) {
            case STREAM_PROGRAM:
                row.type = "program";
                row.program = a;
                row.bankMsb = -1;
                row.bankLsb = b;
                break;
            case STREAM_VOLUME:
                row.type = "volume";
                row.cc = 7;
                row.value = a;
                break;
            case STREAM_PAN:
                row.type = "pan";
                row.cc = 10;
                row.value = a;
                break;
            case STREAM_NOTE_ON:
                row.type = "note_on";
                row.note = a;
                row.velocity = b;
                break;
            case STREAM_NOTE_OFF:
                row.type = "note_off";
                row.note = a;
                row.velocity = 0;
                break;
            case STREAM_PITCH:
                row.type = "pitch_bend";
                row.pitchBend = 8192 + (a - 32) * 256;
                break;
            case STREAM_MODULATION:
                row.type = "modulation";
                row.cc = 1;
                row.value = a;
                break;
            case STREAM_PITCH_RANGE:
                row.type = "pitch_range";
                row.cc = 6;
                row.value = a;
                break;
            case STREAM_EXPRESSION:
                row.type = "expression";
                row.cc = 11;
                row.value = a;
                break;
            case STREAM_END:
                row.type = "end";
                break;
            default:
                row.type = "stream_" + type;
                row.value = a;
                break;
            }
            rows.add(row);
        }
        return rows;
    }

    private static RowList parseMidi(byte[] midi) {
        RowList rows = new RowList();
        if (midi == null || midi.length < 14 || !hasTag(midi, 0, "MThd")) {
            return rows;
        }
        int headerLen = read32(midi, 4);
        int division = read16(midi, 12);
        if (division <= 0 || division > 0x7fff) {
            division = 96;
        }
        int pos = 8 + headerLen;
        int tempo = 500000;
        while (pos + 8 <= midi.length) {
            if (!hasTag(midi, pos, "MTrk")) {
                pos++;
                continue;
            }
            int len = read32(midi, pos + 4);
            int end = pos + 8 + len;
            if (len < 0 || end > midi.length) {
                break;
            }
            tempo = parseMidiTrack(midi, pos + 8, end, division, tempo, rows);
            pos = end;
        }
        rows.sort();
        return rows;
    }

    private static int parseMidiTrack(byte[] midi, int pos, int end, int division,
            int tempo, RowList rows) {
        int tick = 0;
        int lastTick = 0;
        long timeUs = 0;
        int running = 0;
        while (pos < end) {
            VarResult var = readVar(midi, pos, end);
            if (var.next < 0) {
                break;
            }
            tick += var.value;
            timeUs += (long) (tick - lastTick) * (long) tempo / (long) division;
            lastTick = tick;
            pos = var.next;
            if (pos >= end) {
                break;
            }
            int status = midi[pos] & 255;
            if (status < 0x80) {
                if (running == 0) {
                    break;
                }
                status = running;
            } else {
                pos++;
                if (status < 0xf0) {
                    running = status;
                }
            }
            int timeMs = (int) (timeUs / 1000L);
            if (status == 0xff) {
                if (pos >= end) {
                    break;
                }
                int meta = midi[pos++] & 255;
                VarResult len = readVar(midi, pos, end);
                if (len.next < 0 || len.next + len.value > end) {
                    break;
                }
                if (meta == 0x51 && len.value == 3) {
                    tempo = ((midi[len.next] & 255) << 16)
                            | ((midi[len.next + 1] & 255) << 8)
                            | (midi[len.next + 2] & 255);
                    Row row = new Row();
                    row.timeMs = timeMs;
                    row.type = "tempo";
                    row.value = tempo == 0 ? -1 : 60000000 / tempo;
                    rows.add(row);
                }
                pos = len.next + len.value;
                continue;
            }
            if (status == 0xf0 || status == 0xf7) {
                VarResult len = readVar(midi, pos, end);
                if (len.next < 0 || len.next + len.value > end) {
                    break;
                }
                pos = len.next + len.value;
                continue;
            }
            int command = status & 0xf0;
            int channel = status & 15;
            if (command == 0xc0 || command == 0xd0) {
                if (pos >= end) {
                    break;
                }
                int a = midi[pos++] & 255;
                Row row = new Row();
                row.timeMs = timeMs;
                row.channel = channel;
                if (command == 0xc0) {
                    row.type = "program";
                    row.program = a;
                } else {
                    row.type = "pressure";
                    row.value = a;
                }
                rows.add(row);
                continue;
            }
            if (pos + 2 > end) {
                break;
            }
            int a = midi[pos++] & 255;
            int b = midi[pos++] & 255;
            Row row = new Row();
            row.timeMs = timeMs;
            row.channel = channel;
            if (command == 0x80 || (command == 0x90 && b == 0)) {
                row.type = "note_off";
                row.note = a;
                row.velocity = b;
            } else if (command == 0x90) {
                row.type = "note_on";
                row.note = a;
                row.velocity = b;
            } else if (command == 0xb0) {
                row.type = "control";
                row.cc = a;
                row.value = b;
                if (a == 0) {
                    row.type = "bank_msb";
                    row.bankMsb = b;
                } else if (a == 32) {
                    row.type = "bank_lsb";
                    row.bankLsb = b;
                } else if (a == 7) {
                    row.type = "volume";
                } else if (a == 10) {
                    row.type = "pan";
                } else if (a == 11) {
                    row.type = "expression";
                } else if (a == 1) {
                    row.type = "modulation";
                } else if (a == 6) {
                    row.type = "pitch_range";
                }
            } else if (command == 0xe0) {
                row.type = "pitch_bend";
                row.pitchBend = a | (b << 7);
            } else {
                row.type = "midi_" + command;
                row.value = b;
            }
            rows.add(row);
        }
        return tempo;
    }

    private static void computeDurations(RowList rows) {
        for (int i = 0; i < rows.size; i++) {
            Row on = rows.items[i];
            if (!"note_on".equals(on.type)) {
                continue;
            }
            for (int j = i + 1; j < rows.size; j++) {
                Row off = rows.items[j];
                if ("note_off".equals(off.type) && off.channel == on.channel
                        && off.note == on.note) {
                    on.duration = off.timeMs - on.timeMs;
                    break;
                }
            }
        }
    }

    private static VarResult readVar(byte[] data, int pos, int end) {
        int value = 0;
        for (int i = 0; i < 4 && pos < end; i++) {
            int b = data[pos++] & 255;
            value = (value << 7) | (b & 127);
            if ((b & 128) == 0) {
                return new VarResult(value, pos);
            }
        }
        return new VarResult(0, -1);
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

    private static String value(int value) {
        return value < 0 ? "-" : String.valueOf(value);
    }

    private static final class VarResult {
        int value;
        int next;

        VarResult(int v, int n) {
            value = v;
            next = n;
        }
    }

    private static final class Row {
        int timeMs;
        int channel = -1;
        String type = "";
        int note = -1;
        int velocity = -1;
        int program = -1;
        int bankMsb = -1;
        int bankLsb = -1;
        int cc = -1;
        int value = -1;
        int pitchBend = -1;
        int duration = -1;
    }

    private static final class ReasonCounts {
        String[] reasons = new String[16];
        int[] counts = new int[16];
        int size;

        void add(String reason) {
            for (int i = 0; i < size; i++) {
                if (reason.equals(reasons[i])) {
                    counts[i]++;
                    return;
                }
            }
            if (size == reasons.length) {
                String[] nextReasons = new String[reasons.length * 2];
                int[] nextCounts = new int[counts.length * 2];
                System.arraycopy(reasons, 0, nextReasons, 0, reasons.length);
                System.arraycopy(counts, 0, nextCounts, 0, counts.length);
                reasons = nextReasons;
                counts = nextCounts;
            }
            reasons[size] = reason;
            counts[size] = 1;
            size++;
        }

        void print() {
            for (int i = 0; i < size; i++) {
                System.out.println("MLD_DIFF_REASON reason=" + reasons[i]
                        + " count=" + counts[i]);
            }
        }
    }

    private static final class RowList {
        Row[] items = new Row[64];
        int size;

        void add(Row row) {
            if (size == items.length) {
                Row[] next = new Row[items.length * 2];
                System.arraycopy(items, 0, next, 0, items.length);
                items = next;
            }
            items[size++] = row;
        }

        void sort() {
            for (int i = 1; i < size; i++) {
                Row row = items[i];
                int j = i - 1;
                while (j >= 0 && compare(items[j], row) > 0) {
                    items[j + 1] = items[j];
                    j--;
                }
                items[j + 1] = row;
            }
        }

        private int compare(Row a, Row b) {
            if (a.timeMs != b.timeMs) {
                return a.timeMs - b.timeMs;
            }
            return priority(a.type) - priority(b.type);
        }

        private int priority(String type) {
            if ("tempo".equals(type) || "bank_msb".equals(type)
                    || "bank_lsb".equals(type) || "program".equals(type)
                    || isControllerName(type) || "pitch_bend".equals(type)) {
                return 1;
            }
            if ("note_off".equals(type)) {
                return 2;
            }
            if ("note_on".equals(type)) {
                return 3;
            }
            if ("end".equals(type)) {
                return 5;
            }
            return 6;
        }
    }
}
