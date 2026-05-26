/*
 * Copyright (c) 2003 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.adpcm.ym2608;

import vavi.sound.adpcm.Codec;


/**
 * YAMAHA (YM2608) ADPCM Codec
 * 
 * @author <a href="http://www.memb.jp/~dearna/">Masashi Wada</a> (DEARNA)
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 030823 nsano port from c <br>
 */
class Ym2608 implements Codec {

    /** */
    private class State {
        int stepSize = 127;
        int xn;
    }

    /** */
    private State state = new State();

    /** */
    private static final int[] stepsizeTable = {
        57, 57, 57, 57, 77, 102, 128, 153,
        57, 57, 57, 57, 77, 102, 128, 153
    };

//private int ccc = 0;

    /**
     * @param pcm PCM
     * @return ADPCM
     */
    public int encode(int pcm) {

        // �G���R�[�h���� 2
        int dn = pcm - state.xn;
//System.err.printf("%05d: %d, %d, %d\n", ccc, dn, pcm, xn);
        // �G���R�[�h���� 3, 4
        // I = | dn | / Sn ���� An �����߂�B
        // �搔���g�p���Đ����ʂŉ��Z����B
        int i = (int) ((((long) Math.abs(dn)) << 16) / (state.stepSize << 14));
//System.err.printf("%05d: %d\n", ccc, i);
        if (i > 7) {
            i = 7;
        }
        int adpcm = i & 0xff;

        // �G���R�[�h���� 5
        // L3 + L2 / 2 + L1 / 4 + 1 / 8 * stepSize �� 8 �{���Đ������Z
        i = (adpcm * 2 + 1) * state.stepSize / 8;
//System.err.printf("%05d: %d, %d, %d\n", ccc, i, adpcm, state.stepSize);

        // 1 - 2 * L4 -> L4 �� 1 �̏ꍇ�� -1 ��������̂Ɠ���
        if (dn < 0) {
            // - �̏ꍇ�����r�b�g��t����B
            // �G���R�[�h���� 5 �� ADPCM �������ז��ɂȂ�̂ŁA
            // �\���l�X�V���܂ŕۗ������B
            adpcm |= 0x8;
            state.xn -= i;
        } else {
            state.xn += i;
        }
//System.err.printf("%05d: %d, %d\n", ccc, xn, i);

        // �G���R�[�h���� 6
        // �X�e�b�v�T�C�Y�̍X�V
//System.err.printf("%05d: %d, %d, %d\n", ccc, i, adpcm, state.stepSize);
        state.stepSize = (stepsizeTable[adpcm] * state.stepSize) / 64;

        // �G���R�[�h���� 7
        if (state.stepSize < 127) {
            state.stepSize = 127;
        } else if (state.stepSize > 24576) {
            state.stepSize = 24576;
        }
//ccc++;
//if (ccc > 300) {
// System.exit(0);
//}

        return adpcm;
    }

    /**
     * @param adpcm ADPCM (LSB 4 bit �L��)
     * @return PCM 
     */
    public int decode(int adpcm) {

        // �f�R�[�h���� 2, 3
        // L3 + L2 / 2 + L1 / 4 + 1 / 8 * stepSize �� 8 �{���Đ������Z
        int i = ((adpcm & 7) * 2 + 1) * state.stepSize / 8;
        if ((adpcm & 8) != 0) {
            state.xn -= i;
        } else {
            state.xn += i;
        }

        // �f�R�[�h���� 4
        if (state.xn > 32767) {
            state.xn = 32767;
        } else if (state.xn < -32768) {
            state.xn = -32768;
        }
        // �f�R�[�h���� 5
        state.stepSize = state.stepSize * stepsizeTable[adpcm] / 64;

        // �f�R�[�h���� 6
        if (state.stepSize < 127) {
            state.stepSize = 127;
        } else if (state.stepSize > 24576) {
            state.stepSize = 24576;
        }

        // PCM �ŕۑ�����
        return state.xn;
    }
}

/* */
