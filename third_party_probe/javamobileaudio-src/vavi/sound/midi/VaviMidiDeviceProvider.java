/*
 * Copyright (c) 2004 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.midi;


import javax.sound.midi.MidiDevice;
import javax.sound.midi.spi.MidiDeviceProvider;

import vavi.util.Debug;


/**
 * VaviMidiDeviceProvider.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 041222 nsano initial version <br>
 */
public class VaviMidiDeviceProvider extends MidiDeviceProvider {

    /** ����Ɏg�p */
    public final static int MANUFACTURER_ID = 0x5f;

    /** */
    private static final MidiDevice.Info[] infos = new MidiDevice.Info[] { VaviSequencer.info };

    /* */
    public MidiDevice.Info[] getDeviceInfo() {
        return infos;
    }

    /** ADPCM �Đ��@�\��t������ MIDI �V�[�P���T��Ԃ��܂��B */
    public MidiDevice getDevice(MidiDevice.Info info)
        throws IllegalArgumentException {

Debug.println("��1 info: " + info);
        if (info == VaviSequencer.info) {
            VaviSequencer wrappedSequencer = new VaviSequencer();
            return wrappedSequencer;
        } else {
Debug.println("��1 here");
            throw new IllegalArgumentException();
        }
    }
}

/* */
