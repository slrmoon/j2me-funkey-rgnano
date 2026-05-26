/*
 * Copyright (c) 2006 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.adpcm;

import java.io.FilterOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteOrder;

import javax.sound.sampled.AudioFormat;

import vavi.io.BitOutputStream;


/**
 * AdpcmOutputStream.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 030827 nsano initial version <br>
 *          0.10 060427 nsano refactoring <br>
 */
public abstract class AdpcmOutputStream extends FilterOutputStream {

    /** #write(int) �ɓn�� PCM �̃t�H�[�}�b�g */
    protected AudioFormat.Encoding encoding = AudioFormat.Encoding.PCM_SIGNED;

    /** #write(int) �ɓn�� PCM �̃o�C�g�I�[�_ */
    protected ByteOrder byteOrder;

    /** �G���R�[�_ */
    protected Codec encoder;

    /** */
    protected abstract Codec getCodec();

    /**
     * TODO BitOutputStream �̈������܂Ƃ߂� BitOutputStream �ɂ���H
     * @param out ADPCM
     * @param byteOrder {@link #write(int)} �̃o�C�g�I�[�_ 
     * @param bits {@link BitOutputStream} �̃T�C�Y
     * @param bitOrder {@link BitOutputStream} �̃o�C�g�I�[�_ 
     */
    public AdpcmOutputStream(OutputStream out, ByteOrder byteOrder, int bits, ByteOrder bitOrder) {
        super(new BitOutputStream(out, bits, bitOrder));
        this.byteOrder = byteOrder;
        this.encoder = getCodec();
//Debug.println(this.out);
    }

    /** �c���Ă��Ȃ����ǂ��� (PCM L or H �Е��ێ����Ă邩�ǂ���) */
    protected boolean flushed = true;
    /** ���݂̒l (PCM L or H �Е��������ĂȂ����̕ێ��p) */
    protected int current;

    /**
     * @param b PCM H or L byte �� {@link #byteOrder} ���Ɏw�� (LSB 8bit �L��)
     */
    public void write(int b) throws IOException {
        if (!flushed) {

            if (ByteOrder.BIG_ENDIAN.equals(byteOrder)) {
                current |= b & 0x00ff;
            } else {
                current |= (b & 0x00ff) << 8;
            }

            if (AudioFormat.Encoding.PCM_SIGNED.equals(encoding)) {
                if ((current & 0x8000) != 0) {
                    current -= 0x10000;
                }
            }
//System.err.println("current: " + StringUtil.toHex4(current));
            out.write(encoder.encode(current)); // BitOutputStream write 4bit

            flushed = true;
        } else {
            if (ByteOrder.BIG_ENDIAN.equals(byteOrder)) {
                current = (b & 0xff) << 8;
            } else {
                current = b & 0xff;
            }

            flushed = false;
        }
    }
}

/* */
