/*
 * Copyright (c) 2005 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf.message;

import vavi.sound.smaf.SysexMessage;


/**
 * Realtime Universal System exclusive message.
 * <pre>
 * [MIDI]
 *  FF 7F ... F7
 * </pre>
 * <pre>
 *  0xf0
 *  0xff
 *  SUB-ID#1 | SUB-ID#2 | �͂��炫
 *  ---------+----------+------------------------------------------
 *   00H     | --       | ���g�p
 *   01H     | nn       | MIDI Time Code
 *           | 01H  �@  |  Full Message
 *           | 02H  �@  |  User Bits
 *   02H     | nn       | MIDI Show Control
 *           | 00H  �@  |  MSC Extensions
 *           | 01H�`7FH |  MSC Commands
 *   03H     | nn       | Notation Information
 *           | 01H  �@  |  Bar Number
 *           | 02H  �@  |  Time Signature(Immediate)
 *           | 42H  �@  |  Time Signature(Delayed)
 *   04H     | nn       | Device Control
 *           | 01H  �@  |  Master Volume
 *           | 02H  �@  |  Master Ballance
 *   05H     | nn       | Real Time MTC Cueing
 *           | 00H  �@  |  Special
 *           | 01H  �@  |  Punch In Points
 *           | 02H  �@  |  Punch Out Points
 *           | 03H  �@  |  (Reserved)
 *           | 04H  �@  |  (Reserved)
 *           | 05H  �@  |  Event Start Points
 *           | 06H  �@  |  Event Stop Points
 *           | 07H  �@  |  Event Start Points with additional info.
 *           | 08H  �@  |  Event Stop Points with additional info.
 *           | 09H  �@  |  (Reserved)
 *           | 0AH  �@  |  (Reserved)
 *           | 0BH  �@  |  Cue Points
 *           | 0CH  �@  |  Cue Points with additional info.
 *           | 0DH  �@  |  (Reserved)
 *           | 0EH  �@  |  Event Name in additional info.
 *   06H     | nn       | MIDI Machine Control Commands
 *           | 00H�`7FH |  MMC Commands
 *   07H     | nn       | MIDI Machine Control Responses
 *           | 00H�`7FH |  MMC Commands
 *   08H     | nn       | MIDI Tuning Standard
 *           | 02H   �@ |  Note Change
 * </pre>
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 041227 nsano port from MFi <br>
 * @see "ATS-MA5-SMAF_GL_133_HV.pdf"
 */
public class RealtimeUniversalSysexMessage extends SysexMessage {

    /**

[�}�X�^�[�{�����[��]
      F0 7F 7F 04 01 ll mm F7
          nn=0�`7F(���ʁF0�`127) �� �����l��nn=64 (���ʁF100)

[Master Fine Tuning]
      F0h 7F 7F 04 03 vh vl F7
          �}�X�^�[�E�t�@�C���E�`���[����ݒ肵�܂��B
          A440Hz����̃`���[�j���O���Z���g�P�ʂŎw�肵�܂��B 

[Master Coarse Tuning]
      F0 7F 7F 04 04 00 vl F7 
          �}�X�^�[�E�R�[�X�E�`���[�����ݒ肵�܂��B
          A440Hz����̃`���[�j���O��100[cent]�P�ʂŎw�肵�܂��B

[GM SYSTEM ON]
      F0 7E 7F 09 01 F7

[GM2 SYSTEM ON]
      F0 7E 7F 09 03 F7

[GM SYSTEM OFF]
      F0 7E 7F 09 02 F7 

     */
}

/* */
