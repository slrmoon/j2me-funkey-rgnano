/*
 * Copyright (c) 2006 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.adpcm;

import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteOrder;

import javax.sound.sampled.AudioFormat;

import vavi.io.BitInputStream;
import vavi.io.BitOutputStream;


/**
 * AdpcmInputStream.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 030714 nsano initial version <br>
 *          0.01 030714 nsano fine tune <br>
 *          0.02 030714 nsano fix available() <br>
 *          0.03 030715 nsano read() endian �Ή� <br>
 *          0.10 060427 nsano refactoring <br>
 */
public abstract class AdpcmInputStream extends FilterInputStream {

    /** #read() ���Ԃ� PCM �̃t�H�[�}�b�g */
    protected AudioFormat.Encoding encoding = AudioFormat.Encoding.PCM_SIGNED;

    /** #read() ���Ԃ� PCM �̃o�C�g�I�[�_ */
    protected ByteOrder byteOrder;

    /** �f�R�[�_ */
    protected Codec decoder;

    /** */
    protected abstract Codec getCodec();

    /**
     * @param in PCM
     * @param byteOrder {@link #read()} ���̃o�C�g�I�[�_ 
     * @param bits {@link BitOutputStream} �̃T�C�Y
     * @param bitOrder {@link BitOutputStream} �̃o�C�g�I�[�_ 
     */
    public AdpcmInputStream(InputStream in, ByteOrder byteOrder, int bits, ByteOrder bitOrder) {
        super(new BitInputStream(in, bits, bitOrder));
        this.byteOrder = byteOrder;
        this.decoder = getCodec();
//Debug.println(this.in);
    }

    /** ADPCM (4bit) ���Z���̒��� */
    public int available() throws IOException {
//Debug.println("0: " + in.available() + ", " + ((in.available() * 2) + (rest ? 1 : 0)));
        // TODO * 2 �Ƃ� bits �Ōv�Z���ׂ��H
        return (in.available() * 2) + (rest ? 1 : 0);
    }

    /** �c���Ă��邩�ǂ��� */
    protected boolean rest = false;
    /** ���݂̒l */
    protected int current;

    /**
     * @return PCM H or L (8bit LSB �L��)
     */
    public int read() throws IOException {
//Debug.println(in);
        if (!rest) {
            int adpcm = in.read();
            if (adpcm == -1) {
                return -1;
            }

            current = decoder.decode(adpcm);

            rest = true;
//Debug.println("1: " + StringUtil.toHex2(current & 0xff));
            if (ByteOrder.BIG_ENDIAN.equals(byteOrder)) {
                return (current & 0xff00) >> 8;
            } else {
                return current & 0xff;
            }
        } else {
            rest = false;
//Debug.println("2: " + StringUtil.toHex2((current & 0xff00) >> 8));
            if (ByteOrder.BIG_ENDIAN.equals(byteOrder)) {
                return current & 0xff;
            } else {
                return (current & 0xff00) >> 8;
            }
        }
    }

    /** */
    public int read(byte b[], int off, int len) throws IOException {
        if (b == null) {
            throw new NullPointerException();
        } else if ((off < 0) || (off > b.length) || (len < 0) ||
                 ((off + len) > b.length) || ((off + len) < 0)) {
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
            for (; i < len ; i++) {
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
}

/* */
