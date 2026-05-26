/*
 * Copyright (c) 2005 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf.message.yamaha;

import javax.sound.midi.InvalidMidiDataException;
import javax.sound.midi.MidiEvent;

import vavi.sound.smaf.SysexMessage;
import vavi.sound.smaf.message.MidiContext;
import vavi.sound.smaf.message.MidiConvertible;


/**
 * YamahaMessage.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 050501 nsano initial version <br>
 */
public class YamahaMessage extends SysexMessage
    implements MidiConvertible {

    /**
     * 
     * <li>[MA-3] �X�g���[��PCM �y�A
     * <p>
     * �w�肵����̃X�g���[��PCM�𓯊�����������悤�ݒ�ł��܂��B
     * �������b�Z�[�W����M��A�����ꂩ�̃m�[�g�E�I���œ�̃T�E���h�������ɔ�������܂��B
     * </p>
     * <pre>
     * ex.)F0 xx 43 79 06 7F 08 cl id1 id2 F7
     *  �@�@cl=00(����),01(����)
     *    �@id1=00�`20(Wave ID 1)
     *    �@id2=00�`20(Wave ID 2)
     * </pre>
     * <li> MA-3/MA-5 �X�g���[��PCM �E�F�[�u�E�p���|�b�g
     * <p>
     * �w�肵���X�g���[��PCM�E�F�[�u�̃X�e���I��ʈʒu��ݒ肵�܂��B
     * </p>
     * <pre>
     * ex.)F0 xx 43 79 06 7F 0B id pp dd F7
     *  �@�@id=00�`20(Wave ID)
     *  �@�@pp=00(�w��),01(�N���A),02(�I�t)
     *  �@�@dd=00�`7F(��ʁFCenter=40)
     * </pre>
     * �� ��x������w�肵���ꍇ�A�N���A���Ȃ�����`�����l���E�p���|�b�g(CC#10)�̎w��͌��ʂ���܂���B
     * <pre>
     * ----------------------
     *  MA-3 �}�X�^�[�E�{�����[��
     *  MA-3 �X�g���[��PCM�y�A
     *  MA-3 �X�g���[��PCM�E�F�[�u�E�p���|�b�g
     *  MA-3 ���荞�ݐݒ�
     *  ----------------------
     * </pre>
<pre>

[???] (puc)
         43 01 80 31 xx F7
                     ~~ �e���|�f�[�^�H�@Mtsu �Ŏw�肵������ 

[???] (my dump)
         43 03 91 18 00 F7
         43 03 91 18 00 F7
         43 03 91 19 10 F7
         43 03 91 1A 32 F7
         43 03 91 1C 76 F7
         43 03 91 1D 98 F7

[???] (puc)
FF F0 05 43 02 80 ** F7
                  ~~ 1 �f���^�^�C��������� msec �炵��

[���F�ݒ�] (puc)
FF F0 13 43 02 01 00 50 72 9B 3F C1 98 4B 3F C0 00 10 21 42 00 F7
                  ~~ ~~  1 �o�C�g�ڂ� 00 2 �o�C�g�ڂ����F�ԍ�

[FMAll4HPS] (smaftool)
         43 03 00 00 47 50 01 25 1B 92 42 A0 14 72 71 00 A0 F7
               ~~ ~~ 1: no, 2: 00 or 0x80

[MA-3 SetVoiceFM(0x1f,0x2f)/MA-3 SetVoiceWT(0x1e)] (smaftool)
         43 79 06 7F 01 xx tt nn

[MA-5 SetVoiceFM(0x1c,0x2a)/MA-5 SetVoiceWT(0x1b)] (smaftool)
         43 79 07 7F 01

[Reset] (smaftool)
         43 79    7F 7F

[Volume] (smaftool)
         43 79    7F 00

[???] (smaftool)
         43 79    7F 07

[MA-3,5 SetWave] (smaftool)
         43 79    7F 03

[�X�g���[��PCM �E�F�[�u�p���|�b�g] (proper)
      F0 43 79 06 7F 0B ii cc dd F7
          ii: WaveID 1�`32 �i1H�`20F�j
          cc: �p���|�b�g�w�� 0�A�N���A 1�A�p���I�t 2
          dd: �p���|�b�g�l0�`127 (00H�`7FH)

[���[�U�[�C�x���g] (proper)
      F0 43 79 06 7F 10 dd F7
          dd: ���[�U�[�C�x���g��� 0�`15 (0H�`FH)

</pre>
     *
     * @see "http://www.music.ne.jp/~puc/mmf_format.html"
     * @see "ATS-MA5-SMAF_GL_133_HV.pdf"
     * @see "http://murachue.ddo.jp/web/softlist.cgi?mode=desc&title=mmftool"
     */
    public MidiEvent[] getMidiEvents(MidiContext context) throws InvalidMidiDataException {

        MidiEvent[] events = new MidiEvent[1];
        javax.sound.midi.SysexMessage sysexMessage = new javax.sound.midi.SysexMessage();
//Debug.println("(" + StringUtil.toHex2(command) + "): " + channel + "ch, " + StringUtil.toHex2(value));
        byte[] temp = new byte[data.length + 1];
        temp[0] = (byte) 0xf0;
        System.arraycopy(data, 0, temp, 1, data.length);
        sysexMessage.setMessage(temp, temp.length);
        events[0] = new MidiEvent(sysexMessage, context.getCurrentTick());
        return events;
    }
}

/* */
