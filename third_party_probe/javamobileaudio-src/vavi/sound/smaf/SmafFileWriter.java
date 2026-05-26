/*
 * Copyright (c) 2007 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.logging.Level;

import vavi.util.Debug;


/**
 * SmafFileWriter.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 071012 nsano initial version <br>
 */
class SmafFileWriter {

    /** smaf file types */
    private static int[] types = { SmafFileFormat.FILE_TYPE };

    /** */
    public int[] getSmafFileTypes() {
        return types;
    }

    /** */
    public int[] getSmafFileTypes(Sequence sequence) {
        // sequence �𖳎����Ă��邯�� SMAF Sequence ������^�Ȃ����炢�� 
        return types;
    }

    /** */
    public boolean isFileTypeSupported(int fileType) {
        for (int i = 0; i < types.length; i++) {
            if (types[i] == fileType) {
                return true;
            }
        }
        return false;
    }

    /** */
    public boolean isFileTypeSupported(int fileType, Sequence sequence) {
        // sequence �𖳎����Ă��邯�� SMAF Sequence ������^�Ȃ����炢�� 
        return isFileTypeSupported(fileType);
    }

    /**
     * @param in {@link Sequence#getTracks() Sequence#tracks}[0] ��
     *           �e�� {@link SmafMessage TODO} ��ݒ肷�邱�Ƃ�
     *           �w�b�_�`�����N�̓��e���w�肷�邱�Ƃ��o���܂��B
     * @return 0: fileType ���T�|�[�g����Ă��Ȃ��ꍇ�A�������݃f�[�^�ɃG���[������ꍇ
     *         else: �������񂾃o�C�g��
     */
    public int write(Sequence in, int fileType, OutputStream out)
        throws IOException {

        if (!isFileTypeSupported(fileType)) {
Debug.println(Level.WARNING, "unsupported fileType: " + fileType);
            return 0;
        }

        SmafFileFormat ff = new SmafFileFormat(in);

        // header (�Œ���K�v�Ȃ��̂̓f�t�H���g��ݒ�)
//        try {
//        } catch (InvalidSmafDataException e) {
//            // TODO IOException �ł����̂��H
//            throw (IOException) new IOException().initCause(e);
//        }

        // body
        try {
            ff.writeTo(out);
        } catch (InvalidSmafDataException e) {
Debug.printStackTrace(e);
            return 0;
        }

        return ff.getByteLength();
    }

    /** {@link #write(Sequence, int, OutputStream)} �ɈϏ� */
    public int write(Sequence in, int fileType, File out)
        throws IOException {

        OutputStream os = new BufferedOutputStream(new FileOutputStream(out));
        return write(in, fileType, os);
    }
}

/* */
