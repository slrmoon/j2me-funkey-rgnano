/*
 * Copyright (c) 2002 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.mfi;


/**
 * �`�����l���i���o�Ɉˑ����� {@link MfiMessage} ��\���N���X�ł��B
 * <p>
 * MIDI �ɂ��킹�邽�߂ɂ��̖��O�� Voice ... �̑���ɗp���Ă��܂��B
 * </p>
 * <li>javax.sound.midi �p�b�P�[�W�ɂ͂Ȃ�... ����̂��H
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 031203 nsano initial version <br>
 */
public interface ChannelMessage {

    /** */
    int getVoice();

    /** */
    void setVoice(int voice);
}

/* */
