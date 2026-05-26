/*
 * Copyright (c) 2003 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.io;

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteOrder;

import vavi.util.Debug;


/**
 * Bit �P�ʂœǂݍ��ރX�g���[���ł��D
 * 
 * @todo ���r���[�ȃr�b�g
 * TODO endian ���������Ă��ˁH
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 030713 nsano initial version <br>
 *          0.01 030714 nsano fix available() <br>
 *          0.02 030715 nsano read() BitOrder �Ή� <br>
 *          0.03 030716 nsano 2bit �Ή� <br>
 */
public class BitInputStream extends FilterInputStream {

    /** �r�b�g�� */
    private int bits = 4;

    /** �r�b�g�I�[�_ */
    private ByteOrder bitOrder = ByteOrder.BIG_ENDIAN;

    /**
     * Bit �P�ʂœǂݍ��ރX�g���[�����쐬���܂��D 4Bit, �r�b�O�G���f�B�A���D
     */
    public BitInputStream(InputStream in) {
        this(in, 4, ByteOrder.BIG_ENDIAN);
    }

    /**
     * Bit �P�ʂœǂݍ��ރX�g���[�����쐬���܂��D �r�b�O�G���f�B�A���D
     */
    public BitInputStream(InputStream in, int bits) {
        this(in, bits, ByteOrder.BIG_ENDIAN);
    }

    /** MSB �������Ă��܂��B */
    private int mask;

    /**
     * Bit �P�ʂœǂݍ��ރX�g���[�����쐬���܂��D
     */
    public BitInputStream(InputStream in, int bits, ByteOrder bitOrder) {
        super(in);
        if (bits != 4 && bits != 2) {
            throw new IllegalArgumentException("currently, only supported 2, 4 bit reading");
        }
        this.bits = bits;
        this.bitOrder = bitOrder;

        for (int i = 0; i < bits; i++) {
            mask |= (0x80 >> i);
        }
// Debug.println(bits + ", " + StringUtil.toBits(mask, 8));
// Debug.println(bits + ", " + StringUtil.toBits(mask >> 4, 8));
    }

    /** �c���Ă���r�b�g�� */
    private int restBits = 0;

    /** �r�b�O�G���f�B�A�� */
    private int current;

    /** */
    public int available() throws IOException {
        return (in.available() * (8 / bits)) + (restBits / bits);
    }

    /**
     * �w�肵�� bit �ǂݍ��݂܂��D
     */
    public int read() throws IOException {

        if (restBits == 0) {
            current = in.read();
            if (current == -1) {
                return -1;
            }

            if (ByteOrder.LITTLE_ENDIAN.equals(bitOrder)) {
// Debug.println("B: " + StringUtil.toHex2(current) + ": " +
// StringUtil.toBits(current, 8));
                current = convertEndian(current);
// Debug.println("A: " + StringUtil.toHex2(current) + ": " +
// StringUtil.toBits(current, 8));
            }
            restBits = 8;
        }

        int c = (current & (mask >> (8 - restBits))) >> (restBits - bits);
        restBits -= bits;

// Debug.println("R: " + StringUtil.toHex2(c) + ": " +
// StringUtil.toBits(c, 4));
        return c;
    }

    /**
     * <pre>
     *  2Bit
     *    1    2    3    4	     4    3    2    1
     *  | 01 | 11 | 10 | 01 | -&gt; | 01 | 10 | 11 | 01 |
     * </pre>
     * 
     * @param c 8bit
     */
    private int convertEndian(int c) {
        int max = 8 / bits;
        int r = 0;
        for (int i = 0; i < max; i++) {
            int s1 = i * bits;
            int m = mask >> s1;
            int s2 = (max - 1 - i) * bits;
            int v = (c & m) >> s2;
            r |= v << s1;
        }
        return r;
    }

    /**
     * �������Ȃ��Ƃ��̃N���X�� read ���g�p���Ȃ���������B
     */
    public int read(byte b[], int off, int len) throws IOException {
        if (b == null) {
            throw new NullPointerException();
        } else if ((off < 0) || (off > b.length) || (len < 0) || ((off + len) > b.length) || ((off + len) < 0)) {
            throw new IndexOutOfBoundsException();
        } else if (len == 0) {
            return 0;
        }

        int c = read();
        if (c == -1) {
            return -1;
        }
        b[off] = (byte) c;

        int i = 1;
        try {
            for (; i < len; i++) {
                c = read();
                if (c == -1) {
                    break;
                }
                if (b != null) {
                    b[off + i] = (byte) c;
                }
            }
        } catch (IOException e) {
            e.printStackTrace(System.err);
        }
        return i;
    }

    // -------------------------------------------------------------------------

    /** */
    public static void main(String[] args) throws Exception {
        InputStream is1 = new BufferedInputStream(new FileInputStream(args[0]));
Debug.dump(is1);
        is1.close();
        InputStream is2 = new BitInputStream(new BufferedInputStream(new FileInputStream(args[0])));
Debug.dump(is2);
        is2.close();
    }
}

/* */
