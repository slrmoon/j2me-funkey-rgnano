/*
 * Copyright (c) 2002 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.mfi.vavi;

import java.io.IOException;
import java.io.InputStream;

import javax.sound.midi.InvalidMidiDataException;
import javax.sound.midi.MidiSystem;
import javax.sound.midi.MidiUnavailableException;

import vavi.sound.mfi.InvalidMfiDataException;
import vavi.sound.mfi.MetaEventListener;
import vavi.sound.mfi.MetaMessage;
import vavi.sound.mfi.MfiDevice;
import vavi.sound.mfi.MfiSystem;
import vavi.sound.mfi.MfiUnavailableException;
import vavi.sound.mfi.Sequence;
import vavi.sound.mfi.Sequencer;
import vavi.util.Debug;


/**
 * Sequencer implemented by vavi.
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
 * @version 0.10 020627 nsano midi compliant <br>
 *          1.00 020706 nsano use midi sequencer <br>
 *          1.01 030606 nsano change error trap <br>
 *          1.02 030819 nsano change sequence related <br>
 *          1.03 030902 nsano out source {@link MetaEventListener} <br>
 */
class VaviSequencer implements Sequencer {

    /** the device information */
    private static final MfiDevice.Info info =
        new MfiDevice.Info("Java MFi Sound Sequencer",
                           "Vavisoft",
                           "Software sequencer using midi",
                           "Version " + VaviMfiDeviceProvider.version) {};

    /** sound source of this sequencer */
    private javax.sound.midi.Sequencer midiSequencer;

    /** the sequence of MFi */
    private Sequence sequence;

    /* */
    public MfiDevice.Info getDeviceInfo() {
        return info;
    }

    /* */
    public void close() {
        midiSequencer.removeMetaEventListener(mel);
        midiSequencer.removeMetaEventListener(mea);
        midiSequencer.close();
Debug.println("��0 close: " + midiSequencer.hashCode());
    }

    /* */
    public boolean isOpen() {
        return midiSequencer.isOpen();
    }

    /** ADPCM sequencer */
    private javax.sound.midi.MetaEventListener mea = new MetaEventAdapter();

    /*
     * {@link javax.sound.midi.MidiSystem} ��
     * �g�p���Ă��邽�� javax.sound.midi SPI �̃v���O�������Ŏg�p���Ă͂����܂���B 
     */
    public void open() throws MfiUnavailableException {
        try {
            this.midiSequencer = MidiSystem.getSequencer();
Debug.println("��0 midiSequencer: " + midiSequencer);
            midiSequencer.open();
Debug.println("��0 open: " + midiSequencer.hashCode());
            midiSequencer.addMetaEventListener(mel);
            midiSequencer.addMetaEventListener(mea);
        } catch (MidiUnavailableException e) {
Debug.printStackTrace(e);
            throw (MfiUnavailableException) new MfiUnavailableException().initCause(e);
        }
    }

    /** */
    public void setSequence(Sequence sequence)
        throws InvalidMfiDataException {

        this.sequence = sequence;

        try {
            midiSequencer.setSequence(MfiSystem.toMidiSequence(sequence));
        } catch (InvalidMidiDataException e) {
Debug.println(e);
            throw (InvalidMfiDataException) new InvalidMfiDataException().initCause(e);
        } catch (MfiUnavailableException e) {
Debug.println(e);
            throw (RuntimeException) new IllegalStateException().initCause(e);
        }
    }

    /** */
    public void setSequence(InputStream stream)
        throws IOException,
               InvalidMfiDataException {

        this.setSequence(MfiSystem.getSequence(stream));
    }

    /** */
    public Sequence getSequence() {
        return sequence;
    }

    /** */
    public void start() {
        midiSequencer.start();
Debug.println("��0 start: " + midiSequencer.hashCode());
    }

    /** */
    public void stop() {
        midiSequencer.stop();
Debug.println("��0 stop: " + midiSequencer.hashCode());
    }

    /** */
    public boolean isRunning() {
        return midiSequencer.isRunning();
    }

    //-------------------------------------------------------------------------

    /** {@link vavi.sound.mfi.MetaMessage MetaEvent} ���[�e�B���e�B�B */
    private MetaSupport metaSupport = new MetaSupport();

    /** {@link MetaEventListener} ��o�^���܂��B */
    public void addMetaEventListener(MetaEventListener l) {
        metaSupport.addMetaEventListener(l);
    }

    /** {@link MetaEventListener} ���폜���܂��B */
    public void removeMetaEventListener(MetaEventListener l) {
        metaSupport.removeMetaEventListener(l);
    }

    /** {@link vavi.sound.mfi.MetaMessage MetaEvent} */
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
                    metaMessage.setMessage(0x2f, new byte[0], 0);
                    fireMeta(metaMessage);
                } catch (InvalidMfiDataException e) {
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
