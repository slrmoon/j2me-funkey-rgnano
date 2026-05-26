/*
 * Copyright (c) 2003 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.io;

import java.io.FilterOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteOrder;

import vavi.util.Debug;


/**
 * Bit �P�ʂŏ������ރX�g���[���ł��D
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 030713 nsano initial version <br>
 */
public class BitOutputStream extends FilterOutputStream {

    /** �r�b�g�� */
    private int bits = 4;
    /** �r�b�g�I�[�_ */
    private ByteOrder bitOrder = ByteOrder.BIG_ENDIAN;

    /**
     * Bit �P�ʂœǂݍ��ރX�g���[�����쐬���܂��D
     * 4Bit, �r�b�O�G���f�B�A���D
     */
    public BitOutputStream(OutputStream out) {
        this(out, 4, ByteOrder.BIG_ENDIAN);
    }
    
    /**
     * Bit �P�ʂœǂݍ��ރX�g���[�����쐬���܂��D
     * �r�b�O�G���f�B�A���D
     */
    public BitOutputStream(OutputStream out, int bits) {
        this(out, bits, ByteOrder.BIG_ENDIAN);
    }

    /** LSB �������Ă��܂��B */
    private int mask;

    /**
     * Bit �P�ʂŏ������ރX�g���[�����쐬���܂��D
     */
    public BitOutputStream(OutputStream out, int bits, ByteOrder bitOrder) {
	super(out);
if (bits != 4 && bits != 2) {
 throw new IllegalArgumentException("currently, only supported 2, 4 bit reading");
}
        this.bits = bits;
        this.bitOrder = bitOrder;

        for (int i = 0; i < bits; i++) {
            mask |= (0x01 << i);
        }
//Debug.println(bits + ", " + StringUtil.toBits(mask, 8));
//Debug.println(bits + ", " + StringUtil.toBits(mask << 4, 8));
    }

    /** �ςݏグ��ꂽ�r�b�g�� */
    private int stackedBits = 0;
    /** �r�b�O�G���f�B�A���Őςݏグ�� */
    private int current = 0;

    /**
     * 8 �r�b�g�����g���G���f�B�A���ɕϊ����܂��B
     * <pre>
     * 2Bit
     *   1    2    3    4         4    3    2    1
     * | 01 | 11 | 10 | 01 | -> | 01 | 10 | 11 | 01 |
     * </pre>
     * @param c 8bit
     */
    private int convertEndian(int c) {
        int max = 8 / bits;
        int r = 0;
        for (int i = 0; i < max; i++) {
            int s1 = (max - 1 - i) * bits;
            int m = mask << s1;
            int v = (c & m) >> s1;
            int s2 = i * bits;
            r |= v << s2;
        }
        return r;
    }

    /**
     * �w�肵�� bit �������݂܂��D
     */
    public void write(int b) throws IOException {

        b &= mask;
//Debug.println(StringUtil.toHex4(b) + ": " + StringUtil.toBits(b, 8));
        current |= b << (8 - stackedBits - bits);

        stackedBits += bits;

        if (stackedBits == 8) {
            if (ByteOrder.LITTLE_ENDIAN.equals(bitOrder)) {
//Debug.println("B: " + StringUtil.toHex4(current) + ": " + StringUtil.toBits(current, 8));
                current = convertEndian(current);
//Debug.println("A: " + StringUtil.toHex4(current) + ": " + StringUtil.toBits(current, 8));
            }
//Debug.println(StringUtil.toHex4(current) + ": " + StringUtil.toBits(current, 8));
            out.write(current);
            stackedBits= 0;
            current = 0;
        }
    }

    /** */
    public void flush() throws IOException {
Debug.println("stacked bits: " + stackedBits);
        super.flush();
    }

    /** */
    public void close() throws IOException {
        if (stackedBits != 0) {
Debug.println("stacked bits: " + stackedBits + " flushed.");
            out.write(current);
        }
        super.close();
    }

    //-------------------------------------------------------------------------

    /** */
    public static void main(String[] args) throws Exception {
    }
}

/* */
