/*
 * Copyright (c) 2003 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.mfi.vavi;

import javax.sound.midi.MidiEvent;
import vavi.sound.mfi.InvalidMfiDataException;
import vavi.sound.mfi.MfiEvent;


/**
 * MfiConvertible
 * <p>
 * ���̂Ƃ�������N���X�� bean �łȂ���΂Ȃ�Ȃ��D
 * (�����Ȃ��̃R���X�g���N�^�����邱��)
 * </p>
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 030905 nsano initial version <br>
 */
public interface MfiConvertible {

    /** TODO �����@���܂����CBeanUtil �����g���Ȃ����H */
    MfiEvent[] getMfiEvents(MidiEvent midiEvent, MfiContext context)
        throws InvalidMfiDataException;
}

/* */
