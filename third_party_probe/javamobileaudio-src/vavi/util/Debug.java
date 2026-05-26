/*
 * Copyright (c) 2001 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.util;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * �f�o�b�O�̃��[�e�B���e�B�N���X�ł��D
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 010821 nsano initial version <br>
 *          0.01 010827 nsano deplecete #assert <br>
 *          0.02 010827 nsano add #getCallerMethod <br>
 *          0.03 010827 nsano add "debug.level" property <br>
 *          0.04 010829 nsano add #getTopCallerMethod <br>
 *          0.05 010906 nsano be public #out <br>
 *          0.06 010918 nsano avoid SecurityException at #init <br>
 *          0.07 020423 nsano add generic #toBits <br>
 *          0.08 020927 nsano add #setOut <br>
 *          0.09 020927 nsano add #dump(byte) <br>
 *          0.10 020927 nsano add #dump(String) <br>
 *          1.00 021027 nsano use logging package <br>
 *          1.01 021027 nsano delete logging unrelated <br>
 *          1.02 021027 nsano why logger has been public ??? <br>
 *          1.03 030322 nsano fix print without \n <br>
 *          1.04 030825 nsano dump length supported <br>
 *          1.05 040102 nsano add #println(char) <br>
 *          1.06 040118 nsano add #dump(byte[], int, int) <br>
 */
public final class Debug {

    /**
     * ���S�Ƀf�o�b�O�R�[�h����菜���ꍇ�͈ȉ��� false ��
     * ���Ă��ׂĂ��ăR���p�C�����Ă��������D
     */
    private static final boolean isDebug = true;
//    private static final boolean isDebug = false;

    //-------------------------------------------------------------------------

    /** �f�o�b�O���̏o�͐�X�g���[�� */
    private static Logger logger = Logger.getLogger(Debug.class.getName());

    /** */
    private static final String LOGGING_CONFIG_CLASS = "java.util.logging.config.class";

    /** */
    private static final String LOGGING_CONFIG_FILE = "java.util.logging.config.file";

    /** */
    static {
        String configClass = System.getProperty(LOGGING_CONFIG_CLASS);
        String configFile  = System.getProperty(LOGGING_CONFIG_FILE);
        if (configClass == null && configFile == null) {
//System.err.println("Debug::<clinit>: no configuration specified, use default");
            try {
                Properties props = new Properties();
                props.load(Debug.class.getResourceAsStream("/vavi/util/logging/logging.properties"));
                configClass = props.getProperty("vavi.util.debug.config.class");
                Class.forName(configClass).newInstance();
            } catch (Exception e) {
e.printStackTrace(System.err);
            }
        }
    }
    
    /**
     * �A�N�Z�X�ł��܂���D
     */
    private Debug() {}
    
    /**
     * ���s�t���Ń��b�Z�[�W���o�͂��܂��D
     *
     * @param	level	���̃��b�Z�[�W�̕\�����x��
     * @param	message	�\�����b�Z�[�W
     */
    public static final void println(Level level, Object message) {
        if (isDebug) {
            StackTraceElement ste = getStackTraceElement(0);
            logger.logp(level,
                        StringUtil.getClassName(ste.getClassName()),
                        ste.getMethodName(),
                        message + "\n");
        }
    }
    
    /**
     * ���s�t���Ń��b�Z�[�W���o�͂��܂��D
     *
     * @param	level	���̃��b�Z�[�W�̕\�����x��
     * @param	message	�\�����b�Z�[�W
     * @see	#print(Level, Object)
     */
    public static final void println(Level level, boolean message) {
        if (isDebug) {
            println(level, String.valueOf(message));
        }
    }

    /**
     * ���s�t���Ń��b�Z�[�W���o�͂��܂��D
     *
     * @param	level	���̃��b�Z�[�W�̕\�����x��
     * @param	message	�\�����b�Z�[�W
     * @see	#println(Level, Object)
     */
    public static final void println(Level level, int message) {
        if (isDebug) {
            println(level, String.valueOf(message));
        }
    }

    /**
     * ���s�t���Ń��b�Z�[�W���o�͂��܂��D
     *
     * @param	message	�\�����b�Z�[�W
     * @see	#println(Level, Object)
     */
    public static final void println(Object message) {
        if (isDebug) {
            println(Level.INFO, message);
        }
    }

    /**
     * ���s�t���Ń��b�Z�[�W���o�͂��܂��D
     *
     * @param	message	�\�����b�Z�[�W
     * @see	#println(Level, Object)
     */
    public static final void println(int message) {
        if (isDebug) {
            println(Level.INFO, String.valueOf(message));
        }
    }

    /**
     * ���s�t���Ń��b�Z�[�W���o�͂��܂��D
     *
     * @param	message	�\�����b�Z�[�W
     * @see	#println(Level, Object)
     */
    public static final void println(char message) {
        if (isDebug) {
            println(Level.INFO, String.valueOf(message));
        }
    }

    /**
     * ���s�t���Ń��b�Z�[�W���o�͂��܂��D
     *
     * @param	message	�\�����b�Z�[�W
     * @see	#println(Level, Object)
     */
    public static final void println(boolean message) {
        if (isDebug) {
            println(Level.INFO, String.valueOf(message));
        }
    }

    /**
     * ���s�t���Ń��b�Z�[�W���o�͂��܂��D
     *
     * @param	message	�\�����b�Z�[�W
     * @see	#println(Level, Object)
     */
    public static final void println(double message) {
        if (isDebug) {
            println(Level.INFO, String.valueOf(message));
        }
    }

    /**
     * ���b�Z�[�W���o�͂��܂�.
     *
     * @param	level	���̃��b�Z�[�W�̕\�����x��
     * @param	message	�\�����b�Z�[�W
     */
    public static final void print(Level level, Object message) {
        if (isDebug) {
            StackTraceElement ste = getStackTraceElement(0);
            logger.logp(level,
                        StringUtil.getClassName(ste.getClassName()),
                        ste.getMethodName(),
                        String.valueOf(message));
        }
    }

    /**
     * ���b�Z�[�W���o�͂��܂��D
     *
     * @param	message	�\�����b�Z�[�W
     * @see	#println(Level, Object)
     */
    public static final void print(Object message) {
        if (isDebug) {
            print(Level.INFO, message);
        }
    }

    /**
     * �f�o�b�O���[�h�Ȃ�X�^�b�N�g���[�X���o�͂��܂��D
     * @param	e	exception
     */
    public static final void printStackTrace(Throwable e) {
        if (isDebug) {
            logger.log(Level.INFO, "Stack Trace", e);
        }
    }

    /**
     * �o�C�g�z��� 16 �i���Ń_���v���܂��D
     */
    public static final void dump(byte[] buf) {
        dump(new ByteArrayInputStream(buf));
    }

    /**
     * �o�C�g�z��� 16 �i���Ń_���v���܂��D
     */
    public static final void dump(byte[] buf, int length) {
        dump(new ByteArrayInputStream(buf), length);
    }

    /**
     * �o�C�g�z��� 16 �i���Ń_���v���܂��D
     */
    public static final void dump(byte[] buf, int offset, int length) {
        dump(new ByteArrayInputStream(buf, offset, length));
    }

    /**
     * �X�g���[���� 16 �i���Ń_���v���܂��D
     */
    public static final void dump(InputStream is) {
        StringBuilder sb = new StringBuilder();
        sb.append("\n");
        sb.append(StringUtil.getDump(is));
        print(sb.toString());
    }

    /**
     * �����t�ŃX�g���[���� 16 �i���Ń_���v���܂��D
     * @param length �������钷��
     */
    public static final void dump(InputStream is, int length) {
        StringBuilder sb = new StringBuilder();
        sb.append("dumped ");
        try {
            int available = is.available();
            sb.append(Math.min(length, available));
            sb.append("/");
            sb.append(available);
        }
        catch (IOException e) {
            sb.append(length);
        }
        sb.append(" bytes limied...\n");
        sb.append(StringUtil.getDump(is, length));
        print(sb.toString());
    }

    /**
     * ���ݎ��s���̃v���O�����̏����擾���܂��D
     * <p>
     * �g�p��F
     * <pre>
     * <tt>
     * Debug.println("�Ăь��̃��\�b�h��" + Debug.getCallerMethod() + "�ł�");
     * </tt>
     * </pre>
     * <p>
     */
    public static final String getCallerMethod() {
        return getCallerMethod(0);
    }

    /**
     * ���ݎ��s���̃v���O�����̏����擾���܂��D
     * @param	depth	�Ăяo�����̐[���C0 �ȏ���w�肷��D
     * �Ώۃ��\�b�h�𒼐ڌĂԏꍇ�� 0�C
     * �Ώۃ��\�b�h���Ăԃ��\�b�h���Ăԏꍇ�� 1�C
     * �̂悤�Ɏw�肷��B
     */
    public static final String getCallerMethod(int depth) {
        StackTraceElement ste = getStackTraceElement(depth);
        return format(ste);
    }

    /**
     * �t�H�[�}�b�g���� StackTraceElement �̕������Ԃ��܂��D
     * @param	ste	StackTraceElement
     */
    private static final String format(StackTraceElement ste) {
        StringBuilder sb = new StringBuilder();
//	sb.append(StringUtil.getClassName(ste.getClassName()));
//	sb.append("::");
//	sb.append(ste.getMethodName());
//	sb.append("(");
//	sb.append(ste.getLineNumber());
//	sb.append(")");
        sb.append(ste.getClassName());
        sb.append(".");
        sb.append(ste.getMethodName());
        sb.append("(");
        sb.append(ste.getFileName());
        sb.append(":");
        sb.append(ste.getLineNumber());
        sb.append(")");
        return sb.toString();
    }

    /**
     * ���̃N���X�̂̌Ăяo�������\�b�h��Ԃ��܂��D
     */
    private static final StackTraceElement getStackTraceElement(int depth) {
        Throwable t = new Throwable();
        StackTraceElement[] stes = t.getStackTrace();
//  System.err.println("----");
//  for (int i = 0; i < stes.length; i++) {
//  System.err.println(stes[i]);
//  }
//  System.err.println("----");
        for (int i = stes.length - 2; i >= 0 ; i--) {
            if (stes[i].getClassName().startsWith(Debug.class.getName())) {
                return stes[i + depth + 1];
            }
        }

        return stes[stes.length - 1];
    }

    /**
     * �w�肳�ꂽ�p�b�P�[�W���̌Ăяo�������\�b�h�̍ŏ�ʂ̕������Ԃ��܂��D
     * <code>vavi.xxx</code> �p�b�P�[�W���Ƃ���� <code>
     * getTopCallerMethod("vavi")</code> �Ǝw�肵�܂��D
     */
    public static final String getTopCallerMethod(String packageName) {

        Throwable t = new Throwable();
        StackTraceElement[] stes = t.getStackTrace();

        for (int i = 0; i < stes.length; i++) {
            if (stes[i].getClassName().startsWith(packageName)) {
                return format(stes[i]);
            }
        }

        return "no such package name: " + packageName;
    }
}

/* */
