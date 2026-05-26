/*
 * Copyright (c) 2002 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.mfi.vavi.track;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.Serializable;

import javax.sound.midi.InvalidMidiDataException;
import javax.sound.midi.MetaMessage;
import javax.sound.midi.MidiEvent;

import vavi.sound.mfi.InvalidMfiDataException;
import vavi.sound.mfi.SysexMessage;
import vavi.sound.mfi.vavi.MidiContext;
import vavi.sound.mfi.vavi.MidiConvertible;
import vavi.sound.mfi.vavi.sequencer.MachineDependSequencer;
import vavi.sound.mfi.vavi.sequencer.MfiMessageStore;
import vavi.sound.midi.VaviMidiDeviceProvider;
import vavi.util.Debug;
import vavi.util.StringUtil;


/**
 * Machine depend System exclusive message.
 * <pre>
 *  0xff, 0xff
 * </pre>
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 020703 nsano refine <br>
 *          0.01 030711 nsano add constants <br>
 *          0.02 030711 nsano add {@link #getCarrier()} <br>
 *          0.03 030712 nsano read length as unsigned <br>
 *          0.04 030820 nsano implements {@link Serializable} <br>
 *          0.05 030821 nsano implements {@link MidiConvertible} <br>
 */
public class MachineDependMessage extends SysexMessage
    implements MidiConvertible, Serializable {

    /** */
    protected MachineDependMessage(byte[] message) {
        super(message);
    }

    /** */
    public MachineDependMessage() {
        super(new byte[0]);
    }

    /**
     * ���b�Z�[�W��ݒ肵�܂��B�f�[�^��6�o�C�g�ڂ���̂���(���ۂ̃f�[�^)���w�肵�܂��B
     * @param delta delta time
     * @param message data from 6th byte
     */
    public void setMessage(int delta, byte[] message)
        throws InvalidMfiDataException {

        byte[] tmp = new byte[5 + message.length];
//Debug.println("data: " + message.length);
        tmp[0] = (byte) (delta & 0xff);
        tmp[1] = (byte) 0xff;
        tmp[2] = (byte) 0xff;
        tmp[3] = (byte) ((message.length / 0x100) & 0xff);
        tmp[4] = (byte) ((message.length % 0x100) & 0xff);
//Debug.dump(new ByteArrayInputStream(tmp, 0, 5));
        System.arraycopy(message, 0, tmp, 5, message.length);

//Debug.println("message: " + tmp.length);
        super.setMessage(tmp, tmp.length);
//Debug.dump(new ByteArrayInputStream(this.data, 0, 10));
    }

    /**
     * for {@link vavi.sound.mfi.vavi.TrackMessage}
     * @param is ���ۂ̃f�[�^ (�w�b�_����, data2 ~)
     */
    public static MachineDependMessage readFrom(int delta, int status, int data1, InputStream is)
        throws InvalidMfiDataException,
               IOException {

//Debug.dump(is);
        DataInputStream dis = new DataInputStream(is);

        int length = dis.readUnsignedShort();
//Debug.println("length: " + length);

        byte[] data = new byte[length + 5];

        data[0] = (byte) (delta & 0xff);
        data[1] = (byte) 0xff;                      // normal 0xff
        data[2] = (byte) 0xff;                      // machine depend 0xff
        data[3] = (byte) ((length / 0x100) & 0xff); // length LSB
        data[4] = (byte) ((length % 0x100) & 0xff); // lenght MSB

        dis.readFully(data, 5, length);

        // 0 delta
        // 5 vendor | carrier
        // 6 
        // 7
Debug.println("MachineDepend: " + StringUtil.toHex2(data[0]) + ", " + StringUtil.toHex2(data[5]) + " " + StringUtil.toHex2(data[6]) + " " + StringUtil.toHex2(data[7]) + " " + (data.length > 8 ? StringUtil.toHex2(data[8]) : "") + " " + (data.length > 9 ? StringUtil.toHex2(data[9]) : "") + " " + (data.length > 10 ? StringUtil.toHex2(data[10]) : ""));
        MachineDependMessage message = new MachineDependMessage(data);
        return message;
    }

    /** */
    public int getVendor() {
        return data[5] & 0xf0;
    }

    /** */
    public int getCarrier() {
        return data[5] & 0x0f;
    }

    /** */
    public String toString() {
        return "MachineDepend: " +
            "vendor: " + StringUtil.toHex2(data[5] & 0xff);
    }

    //----

    /**
     * <p>
     * ���� {@link MachineDependMessage} �̃C���X�^���X�ɑΉ�����
     * MIDI ���b�Z�[�W�Ƃ��� Meta type 0x7f �� {@link MetaMessage} ���쐬����B
     * {@link MetaMessage} �̎��f�[�^�Ƃ��� {@link MfiMessageStore}
     * �ɂ��� {@link MachineDependMessage} �̃C���X�^���X���X�g�A���č̔Ԃ��ꂽ id ��
     * 2 bytes big endian �Ŋi�[����B
     * </p>
     * <p>
     * �Đ��̏ꍇ�� {@link javax.sound.midi.MetaEventListener} �� Meta type 0x7f ��
     * ���b�X�����đΉ����� id �̃��b�Z�[�W�� {@link MfiMessageStore} ���猩����B
     * ����� {@link vavi.sound.mfi.vavi.sequencer.MachineDependSequencer} �ɂ����čĐ�������
     * �s���B
     * </p>
     * <p>
     * �Đ��@�\�� vavi.sound.mfi.vavi.MetaEventAdapter ���Q�ƁB
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
     *  0x01 {@link MachineDependMessage} �f�[�^�ł��邱�Ƃ�\��
     *  DH DL �̔Ԃ��ꂽ id
     * </pre>
     * @see vavi.sound.midi.VaviMidiDeviceProvider#MANUFACTURER_ID
     * @see MachineDependSequencer#META_FUNCTION_ID_MACHINE_DEPEND
     */
    public MidiEvent[] getMidiEvents(MidiContext context)
        throws InvalidMidiDataException {

        MetaMessage metaMessage = new MetaMessage();

        int id = MfiMessageStore.put(this);
        byte[] data = {
            VaviMidiDeviceProvider.MANUFACTURER_ID,
            MachineDependSequencer.META_FUNCTION_ID_MACHINE_DEPEND,
            (byte) ((id / 0x100) & 0xff),
            (byte) ((id % 0x100) & 0xff)
        };
        metaMessage.setMessage(0x7f,    // �V�[�P���T�[�ŗL���^�C�x���g
                               data,
                               data.length);

        return new MidiEvent[] {
            new MidiEvent(metaMessage, context.getCurrent())
        };
    }
}

/* */
