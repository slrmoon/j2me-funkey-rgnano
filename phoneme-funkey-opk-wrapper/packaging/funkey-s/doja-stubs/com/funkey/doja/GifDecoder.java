package com.funkey.doja;

import java.io.IOException;

public final class GifDecoder {
    private static final int MAX_STACK_SIZE = 4096;

    private byte[] data;
    private int pos;

    private GifDecoder(byte[] bytes) {
        data = bytes;
    }

    public static DecodedImage decode(byte[] bytes) throws IOException {
        return new GifDecoder(bytes).decodeImage();
    }

    private DecodedImage decodeImage() throws IOException {
        if (data.length < 13 || data[0] != 'G' || data[1] != 'I' || data[2] != 'F') {
            throw new IllegalArgumentException();
        }

        pos = 6;
        int canvasWidth = readShort();
        int canvasHeight = readShort();
        int packed = read();
        boolean hasGlobalTable = (packed & 0x80) != 0;
        int globalTableSize = 2 << (packed & 0x07);
        read();
        read();

        int[] globalTable = null;
        if (hasGlobalTable) {
            globalTable = readColorTable(globalTableSize);
        }

        int transparentIndex = -1;
        boolean hasAlpha = false;

        while (pos < data.length) {
            int block = read();
            if (block == 0x3b) {
                break;
            }
            if (block == 0x21) {
                int label = read();
                if (label == 0xf9) {
                    int size = read();
                    int gcePacked = read();
                    readShort();
                    transparentIndex = read();
                    hasAlpha = (gcePacked & 0x01) != 0;
                    for (int i = 4; i < size; i++) {
                        read();
                    }
                    read();
                } else {
                    skipSubBlocks();
                }
                continue;
            }
            if (block != 0x2c) {
                throw new IOException("Unsupported GIF block");
            }

            int left = readShort();
            int top = readShort();
            int width = readShort();
            int height = readShort();
            int imagePacked = read();
            boolean hasLocalTable = (imagePacked & 0x80) != 0;
            boolean interlaced = (imagePacked & 0x40) != 0;
            int localTableSize = 2 << (imagePacked & 0x07);
            int[] colorTable = hasLocalTable ? readColorTable(localTableSize) : globalTable;
            if (colorTable == null) {
                throw new IOException("Missing GIF color table");
            }

            int lzwMinimumCodeSize = read();
            byte[] compressed = readSubBlocks();
            byte[] pixels = decodeLzw(compressed, lzwMinimumCodeSize, width * height);

            int[] argb = new int[canvasWidth * canvasHeight];
            int src = 0;
            if (interlaced) {
                src = copyInterlaced(pixels, src, argb, canvasWidth, left, top, width, height,
                        colorTable, transparentIndex, hasAlpha);
            } else {
                for (int y = 0; y < height; y++) {
                    src = copyRow(pixels, src, argb, canvasWidth, left, top + y, width,
                            colorTable, transparentIndex, hasAlpha);
                }
            }

            return new DecodedImage(canvasWidth, canvasHeight, argb, hasAlpha);
        }

        throw new IOException("No GIF image");
    }

    private int copyInterlaced(byte[] pixels, int src, int[] argb, int canvasWidth,
            int left, int top, int width, int height, int[] colorTable,
            int transparentIndex, boolean hasAlpha) {
        int[] starts = { 0, 4, 2, 1 };
        int[] steps = { 8, 8, 4, 2 };
        for (int pass = 0; pass < 4; pass++) {
            for (int y = starts[pass]; y < height; y += steps[pass]) {
                src = copyRow(pixels, src, argb, canvasWidth, left, top + y, width,
                        colorTable, transparentIndex, hasAlpha);
            }
        }
        return src;
    }

    private int copyRow(byte[] pixels, int src, int[] argb, int canvasWidth,
            int left, int y, int width, int[] colorTable,
            int transparentIndex, boolean hasAlpha) {
        int dst = y * canvasWidth + left;
        for (int x = 0; x < width && src < pixels.length; x++) {
            int index = pixels[src++] & 0xff;
            if (hasAlpha && index == transparentIndex) {
                argb[dst + x] = 0x00000000;
            } else if (index < colorTable.length) {
                argb[dst + x] = colorTable[index];
            } else {
                argb[dst + x] = 0xff000000;
            }
        }
        return src;
    }

    private byte[] decodeLzw(byte[] input, int minimumCodeSize, int expectedPixels)
            throws IOException {
        int clear = 1 << minimumCodeSize;
        int end = clear + 1;
        int available = clear + 2;
        int oldCode = -1;
        int codeSize = minimumCodeSize + 1;
        int codeMask = (1 << codeSize) - 1;

        short[] prefix = new short[MAX_STACK_SIZE];
        byte[] suffix = new byte[MAX_STACK_SIZE];
        byte[] stack = new byte[MAX_STACK_SIZE + 1];
        byte[] pixels = new byte[expectedPixels];

        for (int i = 0; i < clear; i++) {
            suffix[i] = (byte)i;
        }

        int datum = 0;
        int bits = 0;
        int inputPos = 0;
        int outputPos = 0;
        int top = 0;
        int first = 0;

        while (outputPos < expectedPixels) {
            if (top == 0) {
                while (bits < codeSize) {
                    if (inputPos >= input.length) {
                        return pixels;
                    }
                    datum |= (input[inputPos++] & 0xff) << bits;
                    bits += 8;
                }

                int code = datum & codeMask;
                datum >>= codeSize;
                bits -= codeSize;

                if (code == clear) {
                    codeSize = minimumCodeSize + 1;
                    codeMask = (1 << codeSize) - 1;
                    available = clear + 2;
                    oldCode = -1;
                    continue;
                }
                if (code == end) {
                    break;
                }
                if (code > available) {
                    throw new IOException("Bad GIF LZW code");
                }
                if (oldCode == -1) {
                    pixels[outputPos++] = suffix[code];
                    first = suffix[code] & 0xff;
                    oldCode = code;
                    continue;
                }

                int inCode = code;
                if (code == available) {
                    stack[top++] = (byte)first;
                    code = oldCode;
                }

                while (code > clear) {
                    stack[top++] = suffix[code];
                    code = prefix[code] & 0xffff;
                }

                first = suffix[code] & 0xff;
                stack[top++] = (byte)first;

                if (available < MAX_STACK_SIZE) {
                    prefix[available] = (short)oldCode;
                    suffix[available] = (byte)first;
                    available++;
                    if (available == (1 << codeSize) && codeSize < 12) {
                        codeSize++;
                        codeMask = (1 << codeSize) - 1;
                    }
                }

                oldCode = inCode;
            }

            top--;
            pixels[outputPos++] = stack[top];
        }

        return pixels;
    }

    private int[] readColorTable(int size) throws IOException {
        int[] table = new int[size];
        for (int i = 0; i < size; i++) {
            int r = read();
            int g = read();
            int b = read();
            table[i] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
        return table;
    }

    private byte[] readSubBlocks() throws IOException {
        byte[][] blocks = new byte[32][];
        int count = 0;
        int total = 0;
        while (true) {
            int size = read();
            if (size == 0) {
                break;
            }
            byte[] block = new byte[size];
            for (int i = 0; i < size; i++) {
                block[i] = (byte)read();
            }
            if (count == blocks.length) {
                byte[][] bigger = new byte[blocks.length * 2][];
                System.arraycopy(blocks, 0, bigger, 0, blocks.length);
                blocks = bigger;
            }
            blocks[count++] = block;
            total += size;
        }

        byte[] out = new byte[total];
        int dst = 0;
        for (int i = 0; i < count; i++) {
            System.arraycopy(blocks[i], 0, out, dst, blocks[i].length);
            dst += blocks[i].length;
        }
        return out;
    }

    private void skipSubBlocks() throws IOException {
        while (true) {
            int size = read();
            if (size == 0) {
                return;
            }
            pos += size;
            if (pos > data.length) {
                throw new IOException();
            }
        }
    }

    private int readShort() throws IOException {
        int lo = read();
        int hi = read();
        return lo | (hi << 8);
    }

    private int read() throws IOException {
        if (pos >= data.length) {
            throw new IOException();
        }
        return data[pos++] & 0xff;
    }

    public static final class DecodedImage {
        public final int width;
        public final int height;
        public final int[] argb;
        public final boolean hasAlpha;

        DecodedImage(int w, int h, int[] pixels, boolean alpha) {
            width = w;
            height = h;
            argb = pixels;
            hasAlpha = alpha;
        }
    }
}
