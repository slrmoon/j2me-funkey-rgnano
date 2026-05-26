/*
 * Copyright (c) 2003 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.adpcm.ccitt;

import java.io.OutputStream;
import java.nio.ByteOrder;

import vavi.sound.adpcm.AdpcmOutputStream;
import vavi.sound.adpcm.Codec;


/**
 * G721 OutputStream
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 030827 nsano initial version <br>
 *          0.10 060427 nsano refactoring <br>
 */
public class G721OutputStream extends AdpcmOutputStream {

    /** �G���R�[�_ */
    protected Codec getCodec() {
        return new G721();
    }

    /**
     * {@link vavi.io.BitOutputStream} �� 4bit little endian �Œ�
     * <li> TODO {@link vavi.io.BitOutputStream} �� endian
     * @param out ADPCM
     * @param byteOrder {@link #write(int)} �̃o�C�g�I�[�_ 
     */
    public G721OutputStream(OutputStream out, ByteOrder byteOrder) {
        super(out, byteOrder, 4, ByteOrder.LITTLE_ENDIAN);
        ((G721) encoder).setEncoding(encoding);
    }
}

/* */
