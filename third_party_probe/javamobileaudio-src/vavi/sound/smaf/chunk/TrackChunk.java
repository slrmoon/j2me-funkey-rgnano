/*
 * Copyright (c) 2004 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf.chunk;

import java.util.List;

import vavi.sound.smaf.InvalidSmafDataException;
import vavi.sound.smaf.SmafEvent;


/**
 * TrackChunk.
 *
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 041222 nsano initial version <br>
 */
public abstract class TrackChunk extends Chunk {

    /**
     * <pre>
     * 0:   ma1 
     * 1~4: ma2
     * 5:   ma3
     * 6:   ma5
     * 7:   ma7
     * </pre>
     */
    protected int trackNumber;

    /** */
    public TrackChunk(byte[] id, int size) {
        super(id, size);

        this.trackNumber = id[3] & 0xff;
    }

    /** */
    protected TrackChunk() {
    }

    /** */
    public int getTrackNumber() {
        return trackNumber;
    }

    /** */
    public void setTrackNumber(int trackNumber) {
        this.trackNumber = trackNumber;
        id[3] = (byte) trackNumber;
    }

    /** */
    public enum FormatType {
        /** */
        HandyPhoneStandard(2, false),
        /** �n�t�}���������ɂ�鈳�k���s�� */
        MobileStandard_Compress(16, true),
        /** */
        MobileStandard_NoCompress(16, false);
        /** */
        boolean compressed;
        /** size in file */
        int size;
        /** */
        FormatType(int size, boolean compressed) {
            this.size = size;
            this.compressed = compressed;
        }
    }

    /** */
    protected FormatType formatType;

    /**
     * @return Returns the formatType.
     */
    public FormatType getFormatType() {
        return formatType;
    }

    /**
     * ���̃X�e�[�^�X�ł��� Track Chunk �̎��t�H�[�}�b�g���`����B�f�[�^�E�T�C�Y�팸�̂��߁ALSI
     * Native Format �ł̋L�q��A�����p���t���� Control CPU ��z�肵�āA���̑��̃V�[�P���X�E�t�H�[�}
     * �b�g���L�q�\�Ƃ���BCompress �̓n�t�}���������ɂ�鈳�k���s�����Ƃƒ�߂�B
     */
    public void setFormatType(FormatType formatType) {
        this.formatType = formatType;
    }

    /** */
    public enum SequenceType {
        /**
         * Sequence Data �͂P�̘A�������V�[�P���X�E�f�[�^�ł���BSeek Point �� Phrase List �̓V�[�P��
         * �X���̈Ӗ��̂���ʒu���O������Q�Ƃ���ړI�ŗ��p����B
         */
        StreamSequence,
        /**
         * Sequence Data �͕����̃t���[�Y�f�[�^��A���ŕ\�L�������̂ł���BPhrase List �͊O�������
         * �ʃt���[�Y��F������ׂɗp����B
         */
        SubSequence;
    }

    /** */
    protected SequenceType sequenceType;

    /** */
    public void setSequenceType(SequenceType sequenceType) {
        this.sequenceType = sequenceType;
    }

    /** time bases */
    protected static final int[] timeBaseTable = {
        1, 2, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        10, 20, 40, 50
    };

    /**
     * <pre>
     * - formatType = 0x00
     * 0x00 1 msec
     * 0x01 2 msec
     * 0x02 4 msec
     * 0x03 5 msec
     * 0x04�`0x0F Reserved
     * 0x10 10 msec
     * 0x11 20 msec
     * 0x12 40 msec
     * 0x13 50 msec
     * 0x14�`0xFF Reserved
     * - 
     * </pre>
     * @param timeBase real timeBase [msec]
     * @return index of timeBase
     */
    private int findTimeBase(int timeBase) {
        for (int i = 0; i < timeBaseTable.length; i++) {
            if (timeBase == timeBaseTable[i]) {
                return i;
            }
        }
        throw new IllegalArgumentException(String.valueOf(timeBase));
    }

    /** */
    protected int durationTimeBase;

    /**
     * @return Returns the durationTimeBase [msec].
     */
    public int getDurationTimeBase() {
        return timeBaseTable[durationTimeBase];
    }

    /**
     * Timebase_D �� Timebase_G �͓���ł��邱�ƁB
     * @param durationTimeBase in [msec]
     * @throws IllegalArgumentException wrong durationTimeBase
     */
    public void setDurationTimeBase(int durationTimeBase) {
        this.durationTimeBase = findTimeBase(durationTimeBase);
    }

    /** */
    protected int gateTimeTimeBase;

    /**
     * @return Returns the gateTimeTimeBase [msec].
     */
    public int getGateTimeTimeBase() {
        return timeBaseTable[gateTimeTimeBase];
    }

    /**
     * @param gateTimeTimeBase in [msec]
     * @see #durationTimeBase
     * @throws IllegalArgumentException wrong gateTimeTimeBase
     */
    public void setGateTimeTimeBase(int gateTimeTimeBase) {
        this.gateTimeTimeBase = findTimeBase(gateTimeTimeBase);
    }

    /** */
    protected Chunk sequenceDataChunk;

    /** "[MA]tsq" */
    public void setSequenceDataChunk(SequenceDataChunk sequenceDataChunk) {
        this.sequenceDataChunk = sequenceDataChunk;
        size += sequenceDataChunk.getSize() + 8;
    }

    /**
     * �擪�� MetaMessage (META_MACHINE_DEPEND) ������B 
     * TrackChunk �̏��� Properties �ŕێ����Ă���B
     */
    public abstract List<SmafEvent> getSmafEvents()
        throws InvalidSmafDataException;
}

/* */
