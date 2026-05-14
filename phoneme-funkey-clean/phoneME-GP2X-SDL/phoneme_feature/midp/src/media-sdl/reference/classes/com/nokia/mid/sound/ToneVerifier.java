package com.nokia.mid.sound;

final class ToneVerifier {
    private final byte[] data;
    private int pos;

    private ToneVerifier(byte[] tone) {
        data = tone;
    }

    static void fix(byte[] tone) {
        ToneVerifier instance = new ToneVerifier(tone);
        try {
            instance.parseTone();
        } catch (Exception e) {
            System.err.println("ToneVerifier: " + e.getMessage());
        }
    }

    private int read(int length) {
        int p = pos / 8;
        int bit = pos % 8;
        int d = (data[p] & 255) << 8;
        if (bit + length > 8) {
            d += data[p + 1] & 255;
        }
        pos += length;
        return d >> 16 - bit - length & (1 << length) - 1;
    }

    private void skip(int length) {
        pos += length;
    }

    private void replace8(byte[] dst, int offset, int value) {
        int p = offset / 8;
        int bit = offset % 8;
        if (bit == 0) {
            dst[p] = (byte)value;
            return;
        }
        dst[p] = (byte)(((dst[p] >> 8 - bit) << 8 - bit) | (value >> bit));
        dst[p + 1] = (byte)(((dst[p + 1] << 24 + bit) >>> 24 + bit) |
                ((value << 32 - bit) >>> 24));
    }

    private void parseTone() {
        int charWidth;
        skip(8);
        skip(8);
        if (read(7) == 0x22) {
            charWidth = 16;
            read(1);
            skip(7);
        } else {
            charWidth = 8;
        }

        int songType = read(3);
        if (songType == 1) {
            int len = read(4);
            int i;
            for (i = 0; i < len; i++) {
                skip(charWidth);
            }
        } else if (songType != 2) {
            return;
        }

        int j;
label:
        for (j = read(8); j > 0; --j) {
            skip(3);
            skip(2);
            skip(4);
            int patSpecOffset = pos;
            int len = read(8);
            int k;
            for (k = 0; k < len; k++) {
                switch (read(3)) {
                    case 1:
                        skip(4);
                        skip(3);
                        skip(2);
                        break;
                    case 2:
                        skip(2);
                        break;
                    case 3:
                        skip(2);
                        break;
                    case 4:
                        skip(5);
                        break;
                    case 5:
                        skip(4);
                        break;
                    default:
                        replace8(data, patSpecOffset, k);
                        break label;
                }
            }
        }
    }
}
