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
import vavi.util.StringUtil;


/**
 * Data Chunk.
 * <pre>
 * "Dch*" *: language code
 * </pre>
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 041226 nsano initial version <br>
 */
public class DataChunk extends Chunk {

    /** */
    public DataChunk(byte[] id, int size) {
        super(id, size);

        this.languageCode = id[3] & 0xff;
Debug.println("Data(" + languageCode + "): " + size);
    }

    /** */
    public DataChunk() {
        System.arraycopy("Dch".getBytes(), 0, id, 0, 3);
        this.size = 0;
    }

    /** */
    protected void init(InputStream is, Chunk parent)
        throws InvalidSmafDataException, IOException {

//Debug.println("available: " + available());
        while (available() > 4) { // TODO ����t�@�C���� 0 �ł���
            SubData subData = new SubData(is);
Debug.println(subData);
            subDatum.put(subData.tag, subData);
//Debug.println("SubData: " + subData.tag + ", " + subData.data.length + ", " + available());
        }
        skip(is, available()); // TODO ����t�@�C���Ȃ�K�v�Ȃ�
    }

    /** */
    public void writeTo(OutputStream os) throws IOException {
        DataOutputStream dos = new DataOutputStream(os);

        dos.write(id);
        dos.writeInt(size);

        for (SubData subData : subDatum.values()) {
            subData.writeTo(os);
        }
    }

    /** */
    private Map<String, SubData> subDatum = new TreeMap<String, SubData>();

    /** */
    String getSubDataByTag(String tag) {
        try {
            return new String(subDatum.get(tag).data, "Windows-31J"); // use #getLnaguageCode()
        } catch (UnsupportedEncodingException e) {
            return new String(subDatum.get(tag).data);
        }
    }

    /** */
    void addSubData(String tag, String data) {
        SubData subData;
        try {
            subData = new SubData(tag, data.getBytes("Windows-31J")); // use #getLnaguageCode()
        } catch (UnsupportedEncodingException e) {
            subData = new SubData(tag, data.getBytes());
        }
        subDatum.put(tag, subData);
        size += subData.getSize();
    }

    /** */
    private int languageCode;

    /** */
    public int getLanguageCode() {
        return languageCode;
    }
    
    /** */
    public void setLanguageCode(int languageCode) {
        this.languageCode = languageCode;
        id[3] = (byte) languageCode;
    }
    
    /**
     * �L�q���镶���R�[�h��Unicode �̏ꍇ�A���ꂼ��̕����Q�擪��BOM (�o�C�g�I�[�_�[�}�[�N) ��ݒ�
     * ���邱�ƁBBOM �����̏ꍇ�A�r�b�O�G���f�B�A���Ƃ��ĉ��߂���B
     */
    boolean isUnicode(int languageCode) {
        return languageCode >= 0x20 && languageCode <= 0x25; 
    }
    
    /**
     * <pre>
     * tag  2byte (�Œ�)
     * size 2byte (�Œ�)
     * data n byte (��)
     * </pre>
     */
    class SubData {
        
        /** */
        SubData(InputStream is) throws IOException {
            byte[] temp = new byte[2];
            read(is, temp);
            this.tag = new String(temp);
            
            int size = readShort(is);
            
            this.data = new byte[size];
            read(is, this.data);
        }

        /** */
        SubData(String tag, byte[] data) {
            this.tag = tag;
            this.data = data;
        }

        /** */
        public void writeTo(OutputStream os) throws IOException {
            DataOutputStream dos = new DataOutputStream(os);
            os.write(tag.getBytes());
            dos.writeShort(data.length);
            os.write(data);
        }

        /** get data size in file */
        public int getSize() {
            return 2 + 2 + data.length;
        }

        /** �^�O */
        private String tag;

        /** Data */
        private byte[] data;

        /** */
        public String toString() {
            try {
                return new String(tag) + "(" + data.length + "): lang: " + getLanguageCode() + ": " + new String(data, "Windows-31J");
            } catch (UnsupportedEncodingException e) {
                return new String(tag) + "(" + data.length + "): lang: " + getLanguageCode() + ":\n" + StringUtil.getDump(data);
            }
        }
    }
}

/* */
