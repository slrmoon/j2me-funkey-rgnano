/*
 * Copyright (c) 2004 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf.message;

import javax.sound.midi.MidiEvent;

import vavi.sound.smaf.InvalidSmafDataException;
import vavi.sound.smaf.SmafEvent;


/**
 * SmafConvertible.
 * <p>
 * ���̂Ƃ�������N���X�� bean �łȂ���΂Ȃ�Ȃ��D
 * (�����Ȃ��̃R���X�g���N�^�����邱��)
 * </p>
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 041227 nsano port from MFi <br>
 */
public interface SmafConvertible {

    /** TODO �����@���܂����CBeanUtil �����g���Ȃ����H */
    SmafEvent[] getSmafEvents(MidiEvent midiEvent, SmafContext context) throws InvalidSmafDataException;
}

/* */
