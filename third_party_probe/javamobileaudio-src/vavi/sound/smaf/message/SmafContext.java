/*
 * Copyright (c) 2004 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf.message;

import java.io.File;
import java.io.FileOutputStream;
import java.util.BitSet;
import java.util.NoSuchElementException;
import java.util.logging.Level;

import javax.sound.midi.MetaMessage;
import javax.sound.midi.MidiEvent;
import javax.sound.midi.MidiFileFormat;
import javax.sound.midi.MidiMessage;
import javax.sound.midi.MidiSystem;
import javax.sound.midi.ShortMessage;
import javax.sound.midi.SysexMessage;
import javax.sound.midi.Track;

import vavi.sound.smaf.InvalidSmafDataException;
import vavi.sound.smaf.SmafEvent;
import vavi.sound.smaf.SmafSystem;
import vavi.util.Debug;


/**
 * SMAF context for the converter.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 041227 nsano port from MFi <br>
 */
public class SmafContext implements SmafConvertible {

    /** SMAF �̃g���b�N���̍ő�l */
    public static final int MAX_SMAF_TRACKS = 4;

    //----

    /** @see MidiFileFormat#getType() 0: SMF Format 0, 1: SMF Format 1 */
    private int type;

    /** */
    public int getType() {
        return type;
    }

    /** */
    public void setType(int type) {
        this.type = type;
    }

    /** TODO ���̂Ƃ��� sequence#resolution */
    private int timeBase;

    /** */
    public int getTimeBase() {
        return timeBase;
    }

    /** */
    public void setTimeBase(int timeBase) {
        this.timeBase = timeBase;
    }

    //----

    /** index �� SMAF Track No., �g�p����Ă���� true */
    private boolean[] trackUsed = new boolean[MAX_SMAF_TRACKS];

    /**
     * @param smafTrackNumber smaf track number
     */
    public void setTrackUsed(int smafTrackNumber, boolean trackUsed) {
        this.trackUsed[smafTrackNumber] = trackUsed;
    }

    /**
     * @param smafTrackNumber smaf track number
     */
    public boolean isTrackUsed(int smafTrackNumber) {
        return trackUsed[smafTrackNumber];
    }

    //----

    /**
     * tick �̔{��
     */
    private double scale = 1.0d;

    /** */
    public double getScale() {
        return scale;
    }

    /** */
    public void setScale(float scale) {
Debug.println("scale: " + scale);
        this.scale = scale;
    }

    //----

    /** ���O�� tick, index �� SMAF Track No. */
    private long[] beforeTicks = new long[MAX_SMAF_TRACKS];

    /* init */ {
        for (int i = 0; i < MAX_SMAF_TRACKS; i++) {
            beforeTicks[i] = 0;
        }
    }

    /**
     * @param smafTrackNumber midi channel
     */
    public long getBeforeTick(int smafTrackNumber) {
        return beforeTicks[smafTrackNumber];
    }

    /**
     * @param smafTrackNumber midi track number
     */
    public void setBeforeTick(int smafTrackNumber, long beforeTick) {
        this.beforeTicks[smafTrackNumber] = beforeTick;
    }

    /**
     * @param smafTrackNumber midi track number
     */
    public void incrementBeforeTick(int smafTrackNumber, long delta) {
        this.beforeTicks[smafTrackNumber] += getAdjustedDelta(smafTrackNumber, delta * scale);
    }

    /** @return �␳���� ���^�C�� */
    public int retrieveAdjustedDelta(int smafTrackNumber, long currentTick) {
        return getAdjustedDelta(smafTrackNumber, (currentTick - beforeTicks[smafTrackNumber]) / scale);
    }

    /**
     * @return �␳�Ȃ� ���^�C��
     * TODO ���ł���ł��܂������́H
     */
    private int retrieveDelta(int smafTrackNumber, long currentTick) {
        return (int) Math.round((currentTick - beforeTicks[smafTrackNumber]) / scale);
    }

    /** Math#round() �Ŋۂ߂�ꂽ�덷 */
    private float[] roundedSum = new float[MAX_SMAF_TRACKS];
    
    /** Math#round() �Ŋۂ߂�ꂽ�덷�������l���傫���Ȃ����ꍇ�̕␳ */
    private int getAdjustedDelta(int smafTrackNumber, double floatDelta) {
        int delta = (int) Math.round(floatDelta);
        double rounded = floatDelta - delta;
        roundedSum[smafTrackNumber] += rounded;
        if (roundedSum[smafTrackNumber] >= 1f) {
Debug.println("rounded over 1, plus 1: " + roundedSum[smafTrackNumber] + "[" + smafTrackNumber + "]");
            delta += 1;
            roundedSum[smafTrackNumber] -= 1;
        } else if (roundedSum[smafTrackNumber] <= -1f) {
Debug.println("rounded under -1, minus 1: " + roundedSum[smafTrackNumber] + "[" + smafTrackNumber + "]");
            delta -= 1; 
            roundedSum[smafTrackNumber] += 1;
        }
        return delta;
    }
    
    //----

    /**
     * ��O�� NoteOn ����̎��� (currentTick - beforeTicks[track]) ��
     * �����������邩(�����l�A���܂�؂�̂�)�����߁A���̌����}������
     * NopMessage �̔z���Ԃ��܂��B
     * <pre>
     *     event	index	process
     *   |
     * --+- NoteOn	-2	-> brforeTick
     * ��|
     * �b|
     * ��|- NoteOff	-1	-> noteOffEventUsed[-1] = true
     * �b|
     * ��|
     * --+-
     *   |
     *  -O- NoteOn	midiEventIndex
     *   |
     *   |
     *   |
     * --+-
     * </pre>
     * ��L�}���� 1 �� NopMessage ���}�������B
     */
    public SmafEvent[] getIntervalSmafEvents() {

        int interval = 0;
        int track = 0;
        MidiEvent midiEvent = midiTrack.get(midiEventIndex);

        MidiMessage midiMessage = midiEvent.getMessage();
        if (midiMessage instanceof ShortMessage) {
            // note
            ShortMessage shortMessage = (ShortMessage) midiMessage;
            int channel = shortMessage.getChannel();

            track = retrieveSmafTrack(channel);
            interval = retrieveDelta(track, midiEvent.getTick());
        } else if (midiMessage instanceof MetaMessage && ((MetaMessage) midiMessage).getType() == 81) {
            // tempo
            track = smafTrackNumber;
            interval = retrieveDelta(track, midiEvent.getTick());
Debug.println("interval for tempo[" + smafTrackNumber + "]: " + interval);
        } else if (midiMessage instanceof MetaMessage && ((MetaMessage) midiMessage).getType() == 47) {
            // eot
            track = smafTrackNumber;
            interval = retrieveDelta(track, midiEvent.getTick());
Debug.println("interval for EOT[" + smafTrackNumber + "]: " + interval);
        } else if (midiMessage instanceof SysexMessage) {
            return null;
        } else {
Debug.println(Level.WARNING, "not supported message: " + midiMessage);
            return null;
        }
//if (interval > 255) {
// Debug.println("interval: " + interval + ", " + (interval - 256));
//}
if (interval < 0) {
 // ���肦�Ȃ��͂�
 Debug.println(Level.WARNING, "interval: " + interval);
 interval = 0;
}
        int nopLength = interval / 255;
        if (nopLength == 0) {
            return null;
        }
        SmafEvent[] smafEvents = new SmafEvent[nopLength];
        for (int i = 0; i < nopLength; i++) {
            NopMessage smafMessage = new NopMessage(255);
            smafEvents[i] = new SmafEvent(smafMessage, 0l);	// TODO 0l
            // 255 �� �����ɂ��炵�Ă���
            incrementBeforeTick(track, 255);
        };

//Debug.println(nopLength + " nops inserted");
        return smafEvents;
    }

    /**
     * �O�̃f�[�^(MIDI NoteOn)�����s����Ă���̃�(����)���擾���܂��B
     * �K�����O�� #getIntervalSmafEvents() �����s���ă��� 255 �ȉ���
     * �Ԃ��悤�ɂ��Ă����ĉ������B
     */
    public int getDuration() {

        int delta = 0;
        MidiEvent midiEvent = midiTrack.get(midiEventIndex);

        MidiMessage midiMessage = midiEvent.getMessage();
        if (midiMessage instanceof ShortMessage) {
            // note
            ShortMessage shortMessage = (ShortMessage) midiMessage;
            int channel = shortMessage.getChannel();
            
            delta = retrieveAdjustedDelta(retrieveSmafTrack(channel), midiEvent.getTick()); 
        } else if (midiMessage instanceof MetaMessage && ((MetaMessage) midiMessage).getType() == 81) {
            // tempo
            delta = retrieveAdjustedDelta(smafTrackNumber, midiEvent.getTick()); // TODO smafTrackNumber �ł����̂��H
Debug.println("delta for tempo[" + smafTrackNumber + "]: " + delta);
        } else {
Debug.println("no delta defined for: " + midiMessage);
        }

if (delta > 255) {
 // getIntervalSmafEvents �ŏ�������Ă���͂��Ȃ̂ł��肦�Ȃ�
 Debug.println(Level.WARNING, "��: " + delta + ", " + (delta % 256));
}
        return delta % 256;
    }

    //----

    /** �␳���ꂽ SMAF Pitch ���擾���܂��B sound -45, percussion -35 */
    public int retrievePitch(int channel, int pitch) {
        return pitch - 45 + (channel == MidiContext.CHANNEL_DRUM ? 10 : 0);
    }

    /**
     * SMAF Voice No. ���擾���܂��B
     * @param channel MIDI channel
     */
    public int retrieveVoice(int channel) {
        return channel % 4;
    }

    /**
     * MIDI Channel ���擾���܂��B
     * @param voice SMAF channel
     */
    public int retrieveChannel(int voice) {
	return smafTrackNumber * 4 + voice;
    }

    /**
     * SMAF Track ���擾���܂��B
     * @param channel MIDI channel
     */
    public int retrieveSmafTrack(int channel) {
        return channel / 4;
    }

    //----

    /** ���݂� SMAF �̃g���b�N No. */
    private int smafTrackNumber;

    /** ���݂� SMAF �g���b�N No. ��ݒ肵�܂��B */
    public void setSmafTrackNumber(int smafTrackNumber) {
        this.smafTrackNumber = smafTrackNumber;
    }

    /** ���݂� SMAF �g���b�N No. ���擾���܂��B */
    public int getSmafTrackNumber() {
        return smafTrackNumber;
    }

    /** ���݂� MIDI �g���b�N */
    private Track midiTrack;

    /** ���݂� MIDI �g���b�N��ݒ肵�܂��B */
    public void setMidiTrack(Track midiTrack) {
        this.midiTrack = midiTrack;
        this.noteOffEventUsed = new BitSet(midiTrack.size());
    }

    /** ���݂� MIDI �C�x���g�̃C���f�b�N�X�l */
    private int midiEventIndex;

    /** ���݂� MIDI �C�x���g�̃C���f�b�N�X�l��ݒ肵�܂��B */
    public void setMidiEventIndex(int midiEventIndex) {
        this.midiEventIndex = midiEventIndex;
    }

    /** ���݂� MIDI �C�x���g�̃C���f�b�N�X�l���擾���܂��B */
    int getMidiEventIndex() {
        return midiEventIndex;
    }

    /**
     * ���� channel �Ŏ��� ShortMessage �ł��� MIDI �C�x���g���擾���܂��B
     *
     * @throws NoSuchElementException ���� MIDI �C�x���g���Ȃ�
     * @throws IllegalStateException ���݂̃C�x���g�� ShortMessage �ł͂Ȃ�
     */
    public MidiEvent getNextMidiEvent() throws NoSuchElementException {

        ShortMessage shortMessage = null;

        MidiEvent midiEvent = midiTrack.get(midiEventIndex);
        MidiMessage midiMessage = midiEvent.getMessage();
        if (midiMessage instanceof ShortMessage) {
            shortMessage = (ShortMessage) midiMessage;
        } else {
            throw new IllegalStateException("current is not ShortMessage");
        }

        int channel = shortMessage.getChannel();
        int data1 = shortMessage.getData1();

        for (int i = midiEventIndex + 1; i < midiTrack.size(); i++) {
            midiEvent = midiTrack.get(i);
            midiMessage = midiEvent.getMessage();
            if (midiMessage instanceof ShortMessage) {
                shortMessage = (ShortMessage) midiMessage;
                if (shortMessage.getChannel() == channel &&
                    shortMessage.getCommand() == ShortMessage.NOTE_ON &&
                    shortMessage.getData1() != data1) {
Debug.println("next: " + shortMessage.getChannel() + "ch, " + shortMessage.getData1());
                    return midiEvent;
                }
            }
        }

        throw new NoSuchElementException("no next event of channel: " + channel);
    }

    /**
     * ���ݑI�𒆂� NoteOn �C�x���g�Ƒ΂� NoteOff �C�x���g���擾���܂��B
     * IllegalStateException �̓o�O�g���b�v�̂��߂����Ɏg�p���Ă��������B
     * @see vavi.sound.smaf.message.NoteMessage
     *
     * @throws NoSuchElementException �΂� NoteOff �C�x���g���Ȃ�
     * @throws IllegalStateException ���݂̃C�x���g�� ShortMessage �ł͂Ȃ�
     */
    public MidiEvent getNoteOffMidiEvent() throws NoSuchElementException {

        ShortMessage shortMessage = null;

        MidiEvent midiEvent = midiTrack.get(midiEventIndex);
        MidiMessage midiMessage = midiEvent.getMessage();
        if (midiMessage instanceof ShortMessage) {
            shortMessage = (ShortMessage) midiMessage;
        } else {
            throw new IllegalStateException("current is not ShortMessage");
        }

        int channel = shortMessage.getChannel();
        int data1 = shortMessage.getData1();

        for (int i = midiEventIndex + 1; i < midiTrack.size(); i++) {
            midiEvent = midiTrack.get(i);
            midiMessage = midiEvent.getMessage();
            if (midiMessage instanceof ShortMessage) {
                shortMessage = (ShortMessage) midiMessage;
                if (shortMessage.getChannel() == channel &&
                    shortMessage.getData1() == data1) {

                    noteOffEventUsed.set(i);	// ����t���O on
                    return midiEvent;
                }
            }
        }

        throw new NoSuchElementException(channel + "ch, " + data1);
    }

    /** ���łɏ���ꂽ���ǂ��� */
    private BitSet noteOffEventUsed;

    /** ���łɏ���ꂽ���ǂ������擾���܂��B */
    public boolean isNoteOffEventUsed() {
        return noteOffEventUsed.get(midiEventIndex);
    }

    // SmafConvertible ---------------------------------------------------------

    /** BANK LSB */
    private int[] bankLSB = new int[MidiContext.MAX_MIDI_CHANNELS];
    /** BANK MSB */
    private int[] bankMSB = new int[MidiContext.MAX_MIDI_CHANNELS];

    /** */
    public static final int RPN_PITCH_BEND_SENSITIVITY = 0x0000;
    /** */
    public static final int RPN_FINE_TUNING = 0x0001;
    /** */
    public static final int RPN_COURCE_TUNING = 0x0002;
    /** */
    public static final int RPN_TUNING_PROGRAM_SELECT = 0x0003;
    /** */
    public static final int RPN_TUNING_BANK_SELECT = 0x0004;
    /** */
    public static final int RPN_NULL = 0x7f7f;

    /** RPN LSB */
    private int[] rpnLSB = new int[MidiContext.MAX_MIDI_CHANNELS];
    /** RPN MSB */
    private int[] rpnMSB = new int[MidiContext.MAX_MIDI_CHANNELS];

    /** NRPN LSB */
    private int[] nrpnLSB = new int[MidiContext.MAX_MIDI_CHANNELS];
    /** NRPN MSB */
    private int[] nrpnMSB = new int[MidiContext.MAX_MIDI_CHANNELS];

    /** bank, rpn, nrpn */
    public SmafEvent[] getSmafEvents(MidiEvent midiEvent, SmafContext context)
        throws InvalidSmafDataException {

        ShortMessage shortMessage = (ShortMessage) midiEvent.getMessage();
        int channel = shortMessage.getChannel();
//        int command = shortMessage.getCommand();
        int data1 = shortMessage.getData1();
        int data2 = shortMessage.getData2();

        switch (data1) {
        case 0:		// �o���N�Z���N�g MSB
            bankMSB[channel] = data2;
            break;
        case 32:	// �o���N�Z���N�g LSB
            bankLSB[channel] = data2;
            break;
        case 98:	// NRPN LSB
            nrpnLSB[channel] = data2;
            break;
        case 99:	// NRPN MSB
            nrpnMSB[channel] = data2;
            break;
        case 100:	// RPN LSB
            rpnLSB[channel] = data2;
            break;
        case 101:	// RPN MSB
            rpnMSB[channel] = data2;
            break;
        default:
//Debug.println("not implemented: " + data1);
            break;
        }

        return null;
    }

    //-------------------------------------------------------------------------

    /**
     * Converts the midi file to a smaf file.
     * <pre>
     * usage:
     *  % java SmafContext in_midi_file out_mmf_file
     * </pre>
     */
    public static void main(String[] args) throws Exception {

Debug.println("midi in: " + args[0]);
Debug.println("smaf out: " + args[1]);

    	File file = new File(args[0]);
    	javax.sound.midi.Sequence midiSequence = MidiSystem.getSequence(file);
    	MidiFileFormat midiFileFormat = MidiSystem.getMidiFileFormat(file);
        int type = midiFileFormat.getType();
Debug.println("type: " + type);
        vavi.sound.smaf.Sequence smafSequence = SmafSystem.toSmafSequence(midiSequence, type);

        file = new File(args[1]);
        int r = SmafSystem.write(smafSequence, 0, new FileOutputStream(file));
Debug.println("write: " + r);

        System.exit(0);
    }
}

/* */
