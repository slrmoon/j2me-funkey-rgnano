/*
 * Copyright (c) 2002 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.midi;

import java.io.IOException;
import java.util.Properties;

import vavi.util.Debug;


/**
 * Constants for Midi.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 020703 nsano initial version <br>
 */
public final class MidiConstants {

    /** */
    private MidiConstants() {
    }

    /** */
    private static Properties props = new Properties();

    /** */
    public static String getInstrumentName(int index) {
        return props.getProperty("midi.inst.gm." + index);
    }

    //-------------------------------------------------------------------------

    // ���^�E�C�x���g
    //
    // �����ł́A�������̃��^�E�C�x���g�ɂ��Ē�`���Ȃ����B���ׂẴv��
    // �O���������ׂẴ��^�E�C�x���g���T�|�[�g���Ȃ���΂Ȃ�Ȃ��Ƃ������Ƃ�
    // �͂Ȃ��B�ŏ��ɒ�`���ꂽ���^�E�C�x���g�ɂ͈ȉ��̂��̂�����B

    /**
     * <pre>
     * FF 00 02 ssss  [�V�[�P���X�E�i���o�[]
     * </pre>
     * ���̃I�v�V�����E�C�x���g�́A�g���b�N�̖`���A�C�ӂ� 0 �łȂ��f���^�E�^�C
     * ���̑O�ŁA���C�ӂ̓]���\�� MIDI �C�x���g�̑O�ɒu����Ȃ���΂Ȃ炸
     * �A���ꂪ�V�[�P���X�̃i���o�[����肷��B���̃g���b�N���̃i���o�[�� 
     * 1987 �N�Ă� MMA �~�[�e�B���O�ŋ��c���ꂽ�V Cue ���b�Z�[�W�̒��̃V�[�P���X
     * �E�i���o�[�ɑΉ�����B����́A�t�H�[�}�b�g 2 �� MIDI �t�@�C���ɂ����Ă�
     * �A�e�X�́u�p�^�[���v�����ʂ��邱�Ƃɂ���āACue ���b�Z�[�W��p���Ă���
     * �u�\���O�v�V�[�P���X���p�^�[�����Q�Ƃł���悤�ɂ��邽�߂ɗp������B
     * ID �i���o�[���ȗ�����Ă���Ƃ��́A���̃V�[�P���X�̃t�@�C�����ł̈ʒu
     * [�󒍁F�擪���牽�Ԗڂ̃V�[�P���X��]���f�t�H�[���g�Ƃ��ėp������B
     * �t�H�[�}�b�g 0 �܂��� 1 �� MIDI �t�@�C���ɂ����ẮA�P�̃V�[�P���X����
     * �܂܂�Ȃ����߁A���̃i���o�[�͑� 1 ��(�܂�B���)�g���b�N�Ɋ܂܂�
     * �邱�ƂɂȂ�B�������̃}���`�g���b�N�E�V�[�P���X�̓]�����K�v�ȏꍇ��
     * �A�e�X���قȂ�V�[�P���X�E�i���o�[�����t�H�[�}�b�g�P�̃t�@�C���̃O��
     * �[�v�Ƃ��čs���Ȃ���΂Ȃ�Ȃ��B
     */
    public static final int META_SEQUENCE_NO = 0x00;

    /**
     * <pre>
     * FF 01 len text  [�e�L�X�g�E�C�x���g]
     * </pre>
     * �C�ӂ̑傫������ѓ��e�̃e�L�X�g�B�g���b�N�̂����΂񏉂߂ɁA�g���b�N��
     * �A�Ӑ}����I�[�P�X�g���C�V�����A���̑����[�U�������ɒu�������Ǝv�����
     * �������Ă����Ɨǂ��B�e�L�X�g�E�C�x���g�́A�g���b�N���ł��̑��̎��ɓ���
     * �ĉ̎���L���[�E�|�C���g�̋L�q�Ƃ��ėp���邱�Ƃ��ł���B���̃C�x���g��
     * �̃e�L�X�g�́A�ő���̌݊������m�ۂ��邽�߂ɁA����\�ȃA�X�L�[�E�L��
     * ���N�^�łȂ���΂Ȃ�Ȃ��B�������A���ʃr�b�g��p���鑼�̃L�����N�^�E�R
     * �[�h(�󒍁F�����R�[�h�̂悤�� 2 �o�C�g�E�R�[�h�Ȃ�)���A�g�����ꂽ�L��
     * ���N�^�E�Z�b�g���T�|�[�g���铯���R���s���[�^��̈قȂ�v���O�����ԂŃt
     * �@�C�����������邽�߂ɗp���邱�Ƃ��ł���B��A�X�L�[�E�L�����N�^���T�|
     * �[�g���Ȃ��@���̃v���O�����́A���̂悤�ȃR�[�h�𖳎����Ȃ���΂Ȃ��
     * ���B
     * 
     * (0.06 �ł̒ǉ������j ���^�E�C�x���g�̃^�C�v 01 ���� 0F �܂ł͗l�X�ȃ^
     * �C�v�̃e�L�X�g�E�C�x���g�̂��߂ɗ\�񂳂�Ă���B���̊e�X�͏�L�̃e�L�X
     * �g�E�C�x���g�̓����Əd�����Ă��邪�A�ȉ��̂悤�ɁA�قȂ�ړI�̂��߂ɗp
     * ������B
     */
    public static final int META_TEXT_EVENT = 0x01;

    /**
     * <pre>
     * FF 02 len text  [���쌠�\��]
     * </pre>
     * ���쌠�\�����A����\�ȃA�X�L�[�E�e�L�X�g�Ƃ��Ď��B���̕\���ɂ�(C)
     * �̕����ƁA���앨���s�N�ƁA���쌠���L�Җ��Ƃ��܂܂�Ȃ���΂Ȃ�Ȃ��B��
     * �Ƃ� MIDI �t�@�C���Ɋ���̊y�Ȃ����鎞�ɂ́A���ׂĂ̒��쌠�\������
     * �̃C�x���g�ɒu���āA���ꂪ�t�@�C���̐擪�ɗ���悤�ɂ��Ȃ���΂Ȃ�Ȃ�
     * �B���̃C�x���g�͑�P�g���b�N�E�u���b�N�̍ŏ��̃C�x���g�Ƃ��āA�f���^�E
     * �^�C�� = 0 �Œu����Ȃ���΂Ȃ�Ȃ��B
     */
    public static final int META_COPYRIGHT = 0x02;

    /**
     * <pre>
     * FF 03 len text  [�V�[�P���X���܂��̓g���b�N��]
     * </pre>
     * �t�H�[�}�b�g 0 �̃g���b�N�A�������̓t�H�[�}�b�g 1 �̃t�@�C���̑� 1 �g���b
     * �N�ɂ����ẮA�V�[�P���X�̖��́B���̑��̏ꍇ�́A�g���b�N�̖��́B
     */
    public static final int META_NAME = 0x03;

    /**
     * <pre>
     * FF 04 len text  [�y�햼]
     * </pre>
     * ���̃g���b�N�ŗp������ׂ��y��Ґ��̎�ނ��L�q����B MIDI �̖`���ɒu
     * ����郁�^�E�C�x���g�ƂƂ��ɗp���āA�ǂ� MIDI �`���l���ɂ��̋L�q���K�p
     * ����邩����肷�邱�Ƃ�����B���邢�́A�`���l�������̃C�x���g���̃e�L
     * �X�g�œ��肵�Ă��ǂ��B
     */
    public static final int META_INSTRUMENT = 0x04;

    /**
     * <pre>
     * FF 05 len text  [�̎�]
     * </pre>
     * �̎��B��ʓI�ɂ́A�e���߂����̃C�x���g�̃^�C������n�܂�Ɨ������̎��C
     * �x���g�ƂȂ�B
     */
    public static final int META_LYRICS = 0x05;

    /**
     * <pre>
     * FF 06 len text  [�}�[�J�[]
     * </pre>
     * �ʏ�t�H�[�}�b�g 0 �̃g���b�N�A�������̓t�H�[�}�b�g 1 �̃t�@�C���̑�P�g
     * ���b�N�ɂ���B���n�[�T���L����Z�N�V�������̂悤�ȁA�V�[�P���X�̂��̎�
     * �_�̖��́B(�uFirst Verse�v��)
     */
    public static final int META_MARKER = 0x06;

    /**
     * <pre>
     * FF 07 len text  [�L���[�E�|�C���g]
     * </pre>
     * �X�R�A�̂��̈ʒu�ɂ����āA�t�B�����A���B�f�I�E�X�N���[���A���邢�̓X�e
     * �[�W��ŋN�����Ă��邱�Ƃ̋L�q�B(�u�Ԃ��Ƃɓ˂����ށv�u�����J���v�u��
     * �͒j�ɕ���ł���H�킹��v��)
     */
    public static final int META_QUE_POINT = 0x07;

    /**
     * <pre>
     * FF 2F 00  [�g���b�N�̏I���]
     * </pre>
     * ���̃C�x���g�͏ȗ����邱�Ƃ��ł��Ȃ��B���ꂪ���邱�Ƃɂ���ăg���b�N��
     * �������I���_�����m�ɂȂ�A�g���b�N�����m�Ȓ��������悤�ɂȂ�B�����
     * �g���b�N�����[�v�ɂȂ��Ă�����A������Ă����肷�鎞�ɕK�v�ł���B
     */
    public static final int META_END_OF_TRACK = 0x2f;

    /**
     * <pre>
     * FF 51 03 tttttt  [�e���|�ݒ�(�P�ʂ� ��sec / MIDI �l������)]
     * </pre>
     * ���̃C�x���g�̓e���|�E�`�F���W���w������B�u��sec / MIDI �l�������v��
     * ����������΁u(��sec / MIDI �N���b�N)�� 24 ���� 1�v�ł���B�e���|��
     * �u�� / ���ԁv�ł͂Ȃ��u���� / ���v�ɂ���ė^���邱�ƂŁA SMPTE �^�C���E
     * �R�[�h�� MIDI �^�C���E�R�[�h�̂悤�Ȏ����ԃx�[�X�̓����v���g�R����p��
     * �āA��ΓI�ɐ��m�Ȓ����ԓ����𓾂邱�Ƃ��ł���B���̃e���|�ݒ�œ����
     * �鐳�m���́A120 �� / ���� 4 ���̋Ȃ��I��������Ɍ덷�� 500 ��sec �ȓ�
     * �ɂƂǂ܂�A�Ƃ������̂ł���B���z�I�ɂ́A�����̃C�x���g�� Cue ��
     * ��� MIDI �N���b�N������ʒu�ɂ̂݁A�������ׂ��ł���B���̂��Ƃ́A�ق�
     * �̓����f�o�C�X�Ƃ̌݊�����ۏ؂��悤�A���Ȃ��Ƃ��A���̌����݂𑝂₻��
     * �A�Ƃ������̂ŁA���̌��ʁA���̌`���ŕۑ����ꂽ���q�L����e���|�E�}�b�v
     * �͗e�Ղɂق��̃f�o�C�X�֓]���ł��邱�ƂɂȂ�B
     */
    public static final int META_TEMPO = 0x51;
 
    /**
     * <pre>
     * FF 54 05 hr mn se fr ff  [SMPTE �I�t�Z�b�g(0.06 �ł̒ǉ� -  SMPTE
     * �@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�t�H�[�}�b�g�̋L�q)]
     * </pre>
     * ���̃C�x���g�́A��������΁A�g���b�N�E�u���b�N���X�^�[�g���邱�ƂɂȂ�
     * �Ă��� SMPTE �^�C���������B����́A�g���b�N�̖`���ɒu����Ȃ���΂Ȃ�
     * �Ȃ��B���Ȃ킿�A�C�ӂ� 0 �łȂ��f���^�E�^�C���̑O�ŁA���C�ӂ̓]���\
     * �� MIDI �C�x���g�̑O�ł���B���Ԃ́A MIDI �^�C���E�R�[�h�ƑS�����l�� 
     * SMPTE �t�H�[�}�b�g�ŃG���R�[�h����Ȃ���΂Ȃ�Ȃ��B�t�H�[�}�b�g 1 �̃t
     * �@�C���ɂ����ẮA SMPTE �I�t�Z�b�g�̓e���|�E�}�b�v�ƂƂ��ɃX�g�A����
     * ��K�v������A���̃g���b�N�ɂ����Ă͈Ӗ����Ȃ��Ȃ��B�f���^�E�^�C���̂�
     * �߂ɈقȂ�t���[������\���w�肵�Ă��� SMPTE �x�[�X�̃g���b�N�ɂ�����
     * ���Aff �̃t�B�[���h�͍ו������ꂽ�t���[��(100 ���� 1 �t���[���P��)��
     * �����Ă���B
     */
    public static final int META_SMPTE_OFFSERT = 0x54;

    /**
     * <pre>
     * FF 58 04 nn dd cc bb  [���q�L��]
     * </pre>
     * ���q�L���́A4 �̐����ŕ\�������Bnn �� dd �́A�L�����鎞�̂悤�ɁA���q
     * �L���̕��q�ƕ����\���B����� 2 �̃}�C�i�X��ł���B���Ȃ킿�A2 �͎l��
     * ������\���A3 �͔���������\���A���X�B�p�����[�^ cc �́A1 ���g���m�[���E
     * �N���b�N������� MIDI �N���b�N����\�����Ă���B�p�����[�^ bb �́A MIDI 
     * �l������(24 MIDI �N���b�N)�̒��ɋL����̎O�\�񕪉������������邩��
     * �\�����Ă���B���̃p�����[�^�́A MIDI ��̎l������(24 �N���b�N)��
     * �̂��̂Ƃ��ċL�����A���邢�͕\���㑼�̉����ɑΉ�������悤���[�U��`��
     * ����v���O���������ɐ��������݂��邱�Ƃ��������ꂽ�B
     * 
     * �]���āA6 / 8 ���q�ŁA���g���m�[���͔������� 3 ���ɍ��ނ���ǂ��l����
     * �� 24 �N���b�N�ŁA1 ���߂�����ł� 72 �N���b�N�ɂȂ锏�q�́A16 �i�Ŏ�
     * �̂悤�ɂȂ�B
     * 
     *   FF 58 04 06 03 24 08
     * 
     * ����́A8 ���� 6 ���q��(8 �� 2 �� 3 ��Ȃ̂ŁA06 03�ƂȂ�)�A�t�_�l����
     * �������� 36 MIDI �N���b�N(16�i��24�I)[*2]�ŁA MIDI �l�������ɋL
     * ����̎O�\�񕪉����� 8 �Ή�����Ƃ������Ƃ������Ă���B
     */
    public static final int META_58 = 0x58;

    /**
     * <pre>
     * FF 59 02 sf mi  [����]
     *   sf = -7  �t���b�g�V��
     *   sf = -1  �t���b�g�P��
     *   sf = 0   �n��
     *   sf = 1   �V���[�v�P��
     *   sf = 7   �V���[�v�V��
     * 
     *   mi = 0   ����
     *   mi = 1   �Z��
     * </pre>
     */
    public static final int META_59 = 0x59;

    /**
     * <pre>
     * FF 7F len data  [�V�[�P���T�[���胁�^�E�C�x���g]
     * </pre>
     * ����̃V�[�P���T�[�̂��߂̓��ʂȗv���ɂ��̃C�x���g�E�^�C�v��p���邱��
     * ���ł���B�f�[�^�E�o�C�g�̍ŏ��̂P�o�C�g�̓��[�J�[ID�ł���B��������
     * ����A����͌����p�t�H�[�}�b�g�Ȃ̂ł��邩��A���̃C�x���g�E�^�C�v�̎g
     * �p�����X�y�b�N�{�̂̊g���̕����]�܂����B���̃^�C�v�̃C�x���g�́A����
     * ��B��̃t�@�C���E�t�H�[�}�b�g�Ƃ��ėp���邱�Ƃ�I�������V�[�P���T�[��
     * ����Ďg�p����邩������Ȃ��B�d�l�ڍׂ̃t�H�[�}�b�g���m�肵���V�[�P��
     * �T�[�ɂ����ẮA���̃t�H�[�}�b�g��p����ɂ������ĕW���d�l�����ׂ���
     * ���낤�B
     */
    public static final int META_MACHINE_DEPEND = 0x7f;

    //-------------------------------------------------------------------------

    /**
     * 01H�`1FH 
     * 00H 00H 01H�`00H 1FH 7FH 
     */
    public static final int SYSEX_MAKER_ID_American = 0x01;

    /**
     * 20H�`3FH 
     * 00H 20H 00H�`00H 3FH 7FH 
     */
    public static final int SYSEX_MAKER_ID_European = 0x20;

    /**
     * 40H�`5FH 
     * 00H 40H 00H�`00H 5FH 7FH
     */ 
    public static final int SYSEX_MAKER_ID_Japanese = 0x40;

    /**
     * 60H�`7CH 
     * 00H 60H 00H�`00H 7FH 7FH
     */ 
    public static final int SYSEX_MAKER_ID_Other = 0x60;

    /**
     * 7DH�`7FH 
     */
    public static final int SYSEX_MAKER_ID_Special = 0x70; 

    // 7DH
    // ���[�J�[ ID �� 7DH �́A�w�Z����@�ւȂǂł̌����p�Ɏg�p����� ID �ŁA
    // ��c���ړI�ł̂ݎg�p�ł��܂��B 
    //
    // 7EH
    // 7EH �̓m�����A���^�C�����j�o�[�T���V�X�e���G�N�X�N���[�V�u�ł��B
    // �G�N�X�N���[�V�u�̂����A���[�J�[��@��𒴂��Ă����ł����
    // �֗��ȃf�[�^�]���Ɏg�p����܂��B�Ȃ��A7EH�͎����ԂɊ֌W�̂Ȃ��f�[�^�]���Ɏg�p����܂��B 
    //
    // 7FH
    // 7FH �̓��A���^�C�����j�o�[�T���V�X�e���G�N�X�N���[�V�u�ł��B
    // 7EH �̃m�����A���^�C�����j�o�[�T���V�X�e���G�N�X�N���[�V�u�Ɠ��l�ɁA
    // ���[�J�[��@��𒴂��ăf�[�^������肷��Ƃ��Ɏg�p����A
    // 7FH�ł͎����ԂɊ֌W�̂���f�[�^�]���ɗ��p����܂��B 

    //-------------------------------------------------------------------------
    
    /** */
    static {
        try {
            final Class<?> clazz = MidiConstants.class;
            props.load(clazz.getResourceAsStream("midi.properties"));
        } catch (IOException e) {
Debug.println(e);
            System.exit(1);
        }
    }
}

/* */
