/*
 * Copyright (c) 2007 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf.message;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.Serializable;

import javax.sound.midi.InvalidMidiDataException;
import javax.sound.midi.MetaMessage;
import javax.sound.midi.MidiEvent;

import vavi.sound.midi.VaviMidiDeviceProvider;
import vavi.sound.mobile.AudioEngine;
import vavi.sound.smaf.InvalidSmafDataException;
import vavi.sound.smaf.SmafMessage;
import vavi.sound.smaf.chunk.TrackChunk.FormatType;
import vavi.sound.smaf.sequencer.SmafMessageStore;
import vavi.sound.smaf.sequencer.WaveSequencer;
import vavi.util.Debug;
import vavi.util.StringUtil;


/**
 * WaveMessage.
 * TODO SysexMessage �Ƃ�����Ȃ��́H
 * <pre>
 *  format 0x00
 *   duration   1or2
 *   event      cc oo nnnn
 *              ~~ ~~ ~~~~
 *              |  |  +--- number
 *              |  +------ octave
 *              +--------- channel   
 *   gateTime   1or2
 * </pre>
 *
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 071009 nsano initial version <br>
 */
public class WaveMessage extends SmafMessage
    implements WaveSequencer, MidiConvertible, Serializable {

    /** smaf channel 0 ~ 3 */
    private int channel;

    /** */
    private int number;

    /** */
    private int gateTime;

    /**
     * for reading
     * 
     * @param duration
     * @param data
     * @param gateTime
     */
    public WaveMessage(int duration, int data, int gateTime) {
        this.duration = duration;
        this.channel = (data & 0xc0) >> 6; 
        this.number = data & 0x3f; 
        this.gateTime = gateTime;
    }

    /**
     * for writing
     * 
     * @param duration
     * @param channel smaf channel
     * @param number
     * @param gateTime
     */
    public WaveMessage(int duration, int channel, int number, int gateTime) {
        this.duration = duration;
        this.channel = channel; 
        this.number = number; 
        this.gateTime = gateTime;
    }

    /** */
    public int getChannel() {
        return channel;
    }

    /** */
    public int getNumber() {
        return number;
    }

    /** */
    public int getGateTime() {
        return gateTime;
    }

    /** */
    public String toString() {
        return "Wave:" +
            " duration=" + duration +
            " channel=" + channel  +
            " number=" + number  +
            " gateTime=" + StringUtil.toHex4(gateTime);
    }

    //----

    /* */
    @Override
    public byte[] getMessage() {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        FormatType formatType = FormatType.HandyPhoneStandard; // TODO
        switch (formatType) {
        case HandyPhoneStandard:
            try {
                writeOneToTwo(baos, duration);
            } catch (IOException e) {
                assert false;
            }
            int event = 0;
            event |= (channel & 0x03) << 6;
            event |= number & 0x3f;
            baos.write(event);
            try {
                writeOneToTwo(baos, gateTime);
            } catch (IOException e) {
                assert false;
            }
            break;
        case MobileStandard_Compress:
        case MobileStandard_NoCompress:
            throw new UnsupportedOperationException("not implemented"); // TODO
//            break;
        }
        return baos.toByteArray();
    }

    /* */
    @Override
    public int getLength() {
        return getMessage().length;
    }

    /**
     * <p>
     * ���� {@link WaveMessage} �̃C���X�^���X�ɑΉ�����
     * MIDI ���b�Z�[�W�Ƃ��� Meta type 0x7f �� {@link MetaMessage} ���쐬����B
     * {@link MetaMessage} �̎��f�[�^�Ƃ��� {@link SmafMessageStore}
     * �ɂ��� {@link WaveMessage} �̃C���X�^���X���X�g�A���č̔Ԃ��ꂽ id ��
     * 2 bytes big endian �Ŋi�[����B
     * </p>
     * <p>
     * �Đ��̏ꍇ�� {@link javax.sound.midi.MetaEventListener} �� Meta type 0x7f ��
     * ���b�X�����đΉ����� id �̃��b�Z�[�W�� {@link SmafMessageStore} ���猩����B
     * ����� {@link vavi.sound.smaf.sequencer.WaveSequencer} �ɂ����čĐ�������
     * �s���B
     * </p>
     * <p>
     * �Đ��@�\�� vavi.sound.smaf.MetaEventAdapter ���Q�ƁB
     * </p>
     * <pre>
     * MIDI Meta
     * +--+--+--+--+--+--+--+--+--+--+--+-
     * |ff|7f|LL|ID|DD DD ...
     * +--+--+--+--+--+--+--+--+--+--+--+-
     *  0x7f �V�[�P���T�[�ŗL���^�C�x���g
     *  LL �z���}�� 1 byte �H
     *  ID ���[�J�[ID
     * </pre>
     * <pre>
     * ����
     * +--+--+--+--+--+--+--+
     * |ff|7f|LL|5f|01|DH DL|
     * +--+--+--+--+--+--+--+
     *  0x5f ����ɂ������[�J ID
     *  0x01 {@link WaveMessage} �f�[�^�ł��邱�Ƃ�\��
     *  DH DL �̔Ԃ��ꂽ id
     * </pre>
     * @see vavi.sound.midi.VaviMidiDeviceProvider#MANUFACTURER_ID
     * @see vavi.sound.smaf.sequencer.WaveSequencer#META_FUNCTION_ID_SMAF
     */
    public MidiEvent[] getMidiEvents(MidiContext context)
        throws InvalidMidiDataException {

        MetaMessage metaMessage = new MetaMessage();

        int id = SmafMessageStore.put(this);
        byte[] data = {
            VaviMidiDeviceProvider.MANUFACTURER_ID,
            WaveSequencer.META_FUNCTION_ID_SMAF,
            (byte) ((id / 0x100) & 0xff),
            (byte) ((id % 0x100) & 0xff)
        };
        metaMessage.setMessage(0x7f,    // �V�[�P���T�[�ŗL���^�C�x���g
                               data,
                               data.length);

        return new MidiEvent[] {
            new MidiEvent(metaMessage, context.getCurrentTick())
        };
    }

    /* */
    public void sequence() throws InvalidSmafDataException {
Debug.println("WAVE PLAY: " + number);
        AudioEngine engine = Factory.getAudioEngine();
        engine.start(number);
    }
}

/* */
