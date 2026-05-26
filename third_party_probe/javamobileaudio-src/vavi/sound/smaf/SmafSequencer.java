/*
 * Copyright (c) 2007 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf;

import java.io.IOException;
import java.io.InputStream;

import javax.sound.midi.InvalidMidiDataException;
import javax.sound.midi.MidiSystem;
import javax.sound.midi.MidiUnavailableException;

import vavi.util.Debug;


/**
 * Sequencer implemented for SMAF.
 * <p>
 * ���̃V�[�P���T�N���X�ōĐ�����ꍇ��
 * �V�X�e���v���p�e�B <code>javax.sound.midi.Sequencer</code> �� <code>"#Real Time Sequencer"</code>
 * �𖾎�����悤�ɂ��Ă��������B<code>"Java MIDI(MFi/SMAF) ADPCM Sequencer"</code> ��
 * �f�t�H���g�V�[�P���T�ɂȂ����ꍇ�A{@link #mea}���d�����ēo�^����Ă��܂��܂��B
 * </p>
 * <p>
 * {@link javax.sound.midi.MidiSystem} ��
 * �g�p���Ă��邽�� javax.sound.midi SPI �̃v���O�������Ŏg�p���Ă͂����܂���B 
 * </p>
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 071010 nsano initial version <br>
 */
class SmafSequencer implements Sequencer {

    /** the device information */
    private static final SmafDevice.Info info =
        new SmafDevice.Info("Java SMAF Sound Sequencer",
                           "Vavisoft",
                           "Software sequencer using midi",
                           "Version " + SmafDeviceProvider.version) {};

    /** sound source of this sequencer */
    private javax.sound.midi.Sequencer midiSequencer;

    /** the sequence of SMAF */
    private Sequence sequence;

    /** */
    public SmafDevice.Info getDeviceInfo() {
        return info;
    }

    /* */
    public void close() {
        midiSequencer.removeMetaEventListener(mel);
        midiSequencer.removeMetaEventListener(mea);
        midiSequencer.close();
    }

    /* */
    public boolean isOpen() {
        return midiSequencer.isOpen();
    }

    /** ADPCM sequencer */
    private javax.sound.midi.MetaEventListener mea = new MetaEventAdapter();

    /* */
    public void open() throws SmafUnavailableException {
        try {
            this.midiSequencer = MidiSystem.getSequencer();
            midiSequencer.open();
            midiSequencer.addMetaEventListener(mel);
            midiSequencer.addMetaEventListener(mea);
        } catch (MidiUnavailableException e) {
Debug.printStackTrace(e);
            throw (SmafUnavailableException) new SmafUnavailableException().initCause(e);
        }
    }

    /** */
    public void setSequence(Sequence sequence)
        throws InvalidSmafDataException {

        this.sequence = sequence;

        try {
            midiSequencer.setSequence(SmafSystem.toMidiSequence(sequence));
        } catch (InvalidMidiDataException e) {
Debug.println(e);
            throw (InvalidSmafDataException) new InvalidSmafDataException().initCause(e);
        } catch (SmafUnavailableException e) {
Debug.println(e);
            throw (RuntimeException) new IllegalStateException().initCause(e);
        }
    }

    /** */
    public void setSequence(InputStream stream)
        throws IOException,
               InvalidSmafDataException {

        this.setSequence(SmafSystem.getSequence(stream));
    }

    /** */
    public Sequence getSequence() {
        return sequence;
    }

    /** */
    public void start() {
        midiSequencer.start();
    }

    /** */
    public void stop() {
        midiSequencer.stop();
    }

    /** */
    public boolean isRunning() {
        return midiSequencer.isRunning();
    }

    //-------------------------------------------------------------------------

    /** {@link MetaMessage MetaEvent} ���[�e�B���e�B�B */
    private MetaSupport metaSupport = new MetaSupport();

    /** {@link MetaEventListener} ��o�^���܂��B */
    public void addMetaEventListener(MetaEventListener l) {
        metaSupport.addMetaEventListener(l);
    }

    /** {@link MetaEventListener} ���폜���܂��B */
    public void removeMetaEventListener(MetaEventListener l) {
        metaSupport.removeMetaEventListener(l);
    }

    /** {@link MetaMessage MetaEvent} */
    protected void fireMeta(MetaMessage meta) {
        metaSupport.fireMeta(meta);
    }

    /** meta 0x2f listener */
    private javax.sound.midi.MetaEventListener mel = new javax.sound.midi.MetaEventListener() {
        /** */
        public void meta(javax.sound.midi.MetaMessage message) {
//Debug.println("type: " + message.getType());
            switch (message.getType()) {
            case 0x2f:  // �����I�ɍŌ�ɂ��Ă����
                try {
                    MetaMessage metaMessage = new MetaMessage();
                    metaMessage.setMessage(0x2f, null);
                    fireMeta(metaMessage);
                } catch (InvalidSmafDataException e) {
Debug.println(e);
                }
catch (RuntimeException e) {
Debug.printStackTrace(e);
 throw e;
} catch (Error e) {
Debug.printStackTrace(e);
 throw e;
}
                break;
            }
        }
    };
}

/* */
