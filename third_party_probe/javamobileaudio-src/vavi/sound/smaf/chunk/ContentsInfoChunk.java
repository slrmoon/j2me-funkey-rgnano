/*
 * Copyright (c) 2004 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf.chunk;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.util.Map;
import java.util.TreeMap;

import vavi.sound.smaf.InvalidSmafDataException;
import vavi.util.Debug;


/**
 * ContentsInfo Chunk.
 * <pre>
 * "CNTI"
 * 
 *  Contents Class �F1 byte (�K�{)
 *  Contents Type �F1 byte (�K�{)
 *  Contents Code Type �F1 byte (�K�{)
 *  Copy Status �F1 byte (�K�{)
 *  Copy Counts �F1 byte (�K�{)
 *  Option �Fn byte (Option)
 * </pre>
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 041222 nsano initial version <br>
 */
public class ContentsInfoChunk extends Chunk {

    /** */
    public ContentsInfoChunk(byte[] id, int size) {
        super(id, size);
    }

    /** */
    public ContentsInfoChunk() {
        System.arraycopy("CNTI".getBytes(), 0, id, 0, 4);
        this.size = 5;
    }

    /** */
    protected void init(InputStream is, Chunk parent)
        throws InvalidSmafDataException, IOException {

        this.contentsClass = read(is);
Debug.println("contentsClass: " + contentsClass);
        this.contentsType = read(is);
Debug.println("contentsType: " + contentsType);
        this.contentsCodeType = read(is);
Debug.println("contentsCodeType: " + contentsCodeType);
        this.copyStatus = read(is);
Debug.println("copyStatus: " + copyStatus);
        this.copyCounts = read(is);
Debug.println("copyCounts: " + copyCounts);
    	byte[] option = new byte[size - 5];
    	read(is, option);
Debug.println("option: " + option.length + " bytes (subDatum)");
        int i = 0;
        while (i < option.length) {
//Debug.println(i + " / " + option.length + "\n" + StringUtil.getDump(option, i, option.length - i));
            SubData subData = new SubData(option, i, contentsCodeType);
            subDatum.put(subData.getTag(), subData);
Debug.println("ContentsInfo: subData: " + subData);
            i += 2 + 1 + subData.getData().length + 1; // tag ':' data ','
//Debug.println(i + " / " + option.length + "\n" + StringUtil.getDump(option, i, option.length - i));
        }
    }

    /** */
    public void writeTo(OutputStream os) throws IOException {
        DataOutputStream dos = new DataOutputStream(os);

        dos.write(id);
        dos.writeInt(size);

        dos.writeByte(contentsClass);
        dos.writeByte(contentsType);
        dos.writeByte(contentsCodeType);
        dos.writeByte(copyStatus);
        dos.writeByte(copyCounts);
        for (SubData subData : subDatum.values()) {
            subData.writeTo(os);
        }
    }

    /** */
    public static final int CONTENT_CLASS_YAMAHA = 0x00;
    
    /** */
    private int contentsClass;

    /**
     * �R���e���c�̃N���X��\������B
     * ��PDA �[�����A�����̑��@�\�[���œ��l�̃f�[�^�t�H�[�}�b�g�܂ŗ��p����ꍇ�̋�ʂɗp����B
     */
    public void setContentsClass(int contentsClass) {
        this.contentsClass = contentsClass;
    }

    /** */
    private int contentsCodeType;

    /**
     * <pre>
     * 0x01 ISO 8859-1 (Latin-1) �p��A�t�����X��A�h�C�c�� �C�^���A��A�X�y�C����A�|���g�K����
     * 0x02 EUC-KR(KS) �؍���
     * 0x03 HZ-GB-2312 ������(�ȑ̎�)
     * 0x04 Big5 ������(�ɑ̎�)
     * 0x05 KOI8-R ���V�A��Ȃ�
     * 0x06 TCVN-5773:1993 �x�g�i����
     * 0x07�`0x1F Reserved Reserved
     * 0x20 UCS-2 Unicode
     * 0x21 UCS-4 Unicode
     * 0x22 UTF-7 Unicode
     * 0x23 UTF-8 Unicode
     * 0x24 UTF-16 Unicode
     * 0x25 UTF-32 Unicode
     * 0x26�`0xFF Reserved Reserved
     * </pre>
     * �������AContents Info Chunk ��Option �ɂ����ẮA�ǂ̃^�C�v�� �w,(0x2C)�x ��Option �̃f���~�^�Ƃ�
     * �Ē�`���Ă��邽�߁A�G�X�P�[�v�L�����N�^�Ƃ��Ẵo�b�N�X���b�V���w\(0x5C)�x �ƍ��킹�Ďg�p����B
     * <pre>
     *  �\�L �@�\
     *  \, �g,�h ��\��
     *  \\ \��\��
     *  \ ��������
     * </pre>
     * �f���~�^�𐳂������ʏo���Ȃ����Ƃ��z�肳��镶���R�[�h���ݒ肳��Ă���ꍇ�ɂ�Contents Info
     * Chunk ��Option �Ƀf�[�^���L�q�����AOptional Data Chunk �Ƀf�[�^���L�q������̂Ƃ���B
     */
    public void setContentsCodeType(int contentsCodeType) {
        this.contentsCodeType = contentsCodeType;
    }

    /** */
    private int contentsType;

    /**
     * �R���e���c�̃^�C�v��\������B
     * <pre>
     * 0x00�`0x0F, 0x30�`0x33 ���M�����f�B
     * 0x10�`0x1F, 0x40�`0x42 �J���I�P�n
     * 0x20�`0x2F, 0x50�`0x53 CM �n
     * ��L�ȊO�̖��g�p�l Reserved
     * </pre>
     */
    public void setContentsType(int contentsType) {
        this.contentsType = contentsType;
    }

    /** */
    private int copyCounts;

    /**
     * �R�s�[�񐔂�\������B
     * <pre>
     * Copy Counts Description
     * 0x00�`0xFE Copy Counts
     * 0xFF Copy Counts(255 �ȏ�)
     * </pre>
     * �� �R�s�[�^�ړ����������邽�тɂP�J�E���g�i�߂�B
     */
    public void setCopyCounts(int copyCounts) {
        this.copyCounts = copyCounts;
    }

    /** */
    private int copyStatus;

    /**
     * �R���e���c�̎��R�s�[��`��\������B
     * <pre>
     *         | b7       | b6       | b5       | b4       | b3       | b2       | b1      |  b0
     * --------+----------+----------+----------+----------+----------+----------+---------+-----
     * �s��bit | Reserved | Reserved | Reserved | Reserved | Reserved | �ҏW     | �ۑ�    | �]��
     * </pre>
     * b0 �͓]���̉�/�s�Ab1 �͕ۑ��̉�/�s��b2 �͕ҏW�̉�/�s���`����B(0:�� 1:�s��)
     * Reserved bit �́h1�h�Ŗ��߂�B
     */
    public void setCopyStatus(int copyStatus) {
        this.copyStatus = copyStatus;
    }

    /** */
    private Map<String, SubData> subDatum = new TreeMap<String, SubData>();

    /**
     * @return null when specified sub chunk is not found
     */
    public String getSubDataByTag(String tag) {
        SubData subData = subDatum.get(tag);
        if (subData == null) {
            return null;
        }
        try {
            return new String(subData.getData(), "Windows-31J"); // use contentsCodeType
        } catch (UnsupportedEncodingException e) {
            return new String(subData.getData());
        }
    }

    /**
     * Option (Contents Type 0x00, Contents Class 0x00 ~ 0xFF �p)
     * <p>
     * �W���������A�Ȗ��A�A�[�e�B�X�g���A�쎌/��ȎҖ������i�[����B�K�������\���Ɏg�p������̂ł͂Ȃ��A��
     * �ʃf�[�^�̔F���Ɏg�p������̂ł���B
     * �f�[�^�͉ϒ��Ƃ��A�w�^�O (2byte)�x+�w : (0x3A)�x+�wData�x+�w , (0x2C)�x�ŋ�؂��ċL�q����B�^�O������
     * ���Ƃ���B�^�O��2byte �Œ�̃o�C�g��Ƃ���B
     * �wData�x���ɂ����āw , (0x2C)�x�𕶎��Ƃ��Ďg�p����ꍇ �̃G�X�P�[�v�L�����N�^����4.2.3�ɒ�`����B
     * </p>
     * <pre>
     *  ����           | �^�O�� | Hex
     * ----------------+--------+-----------
     *  �x���_�[��     | VN     | 0x56 0x4E
     *  �L�����A��     | CN     | 0x43 0x4E
     *  �J�e�S���[��   | CA     | 0x43 0x41
     *  �Ȗ�           | ST     | 0x53 0x54
     *  �A�[�e�B�X�g�� | AN     | 0x41 0x4E
     *  �쎌           | WW     | 0x57 0x57
     *  ���           | SW     | 0x53 0x57
     *  �ҋ�           | AW     | 0x41 0x57
     *  Copyright&copy;     | CR     | 0x43 0x52
     *  �Ǘ��Ғc�̖�   | GR     | 0x47 0x52
     *  �Ǘ����       | MI     | 0x4D 0x49
     *  �쐬����       | CD     | 0x43 0x44
     *  �X�V����       | UD     | 0x55 0x44
     * </pre>
     * Unicode �̒ǉ��ɂ�����AOption ���̃f���~�^�����ʂł��Ȃ������R�[�h�����݂��邽��
     * Optional Data Chunk ��ǉ������B
     */
    public void addSubData(String tag, String data) {
        SubData subData;
        try {
            subData = new SubData(tag, data.getBytes("Windows-31J")); // use contentsCodeType
        } catch (UnsupportedEncodingException e) {
            subData = new SubData(tag, data.getBytes());
        }
        subDatum.put(tag, subData);
        size += subData.getSize();
    }
}

/* */
