/*
 * Copyright (c) 2002 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.util;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.io.StreamTokenizer;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Set;


/**
 * ������Ɋւ��郆�[�e�B���e�B�̃N���X�ł��D
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 021027 nsano separated from Debug class <br>
 *          0.01 021202 nsano fix getDump(InputStream) <br>
 *          0.02 021206 nsano fix toBits(int,int) <br>
 *          0.03 021206 nsano use properties file <br>
 *          0.04 021207 nsano fix toBits(int,int) when 32bit <br>
 *          0.05 021216 nsano add getCharDump() <br>
 *          0.06 030128 nsano add paramString() <br>
 *          0.07 030211 nsano add toHex16(long) <br>
 *          0.08 030817 nsano add display <br>
 *          0.09 030817 nsano getDump mark supported <br>
 *          0.10 030825 nsano getDump length supported <br>
 *          0.11 031212 nsano add recursive flag, paramString <br>
 *          0.12 031212 nsano rename display to expand<br>
 *          0.13 031228 nsano add when null, paramString <br>
 *          0.14 040118 nsano add splitCommandLine <br>
 *          0.15 040119 nsano add isHexUpperCase property <br>
 *          0.16 040119 nsano add ignoreStatics property <br>
 *          0.17 040212 nsano add ignoreInnerClasses property <br>
 */
public final class StringUtil {

    /** used by expand ... */
    private static final DateFormat sdf = new SimpleDateFormat();

    /** */
    private StringUtil() {}

    /**
     * �w�肵���I�u�W�F�N�g�̃t�B�[���h�̕�������擾���܂��B
     * @param object �\������I�u�W�F�N�g
     */
    public static final String paramString(Object object) {
        return paramString(object, 0);
    }

    /**
     * �w�肵���I�u�W�F�N�g�̃t�B�[���h�̕������\�����܂��B
     * �t�B�[���h���I�u�W�F�N�g�Ȃ�ċA�I�Ɏ擾���܂��B
     * @param object �\������I�u�W�F�N�g
     */
    public static final String paramStringDeep(Object object) {
        return paramString(object, 1);
    }

    /**
     * �I�u�W�F�N�g���e�L�X�g�_���v���܂��B
     * @param object �\������I�u�W�F�N�g
     * @param depth �ċA�I�̐[���A0 �̏ꍇ�͍ċA���܂���B
     */
    private static final String paramString(Object object, int depth) {

        // null
        if (object == null) {
            return null;
        }

        Class<?> clazz = object.getClass();
//Debug.println("class: " + clazz);

        return paramString(clazz, object, depth);
    }

    /**
     * �I�u�W�F�N�g���e�L�X�g�_���v���܂��B
     * toString() �Ŏg�p����ƕ֗��ł��B
     * @param clazz �I�u�W�F�N�g���w�肵���N���X�Ƃ��Ĉ���
     * @param object �\������I�u�W�F�N�g
     * @param depth �ċA�I�̐[���A0 �̏ꍇ�͍ċA���܂���B
     */
    private static final String paramString(Class<?> clazz,
                                            Object object,
                                            int depth) {

        // �\�����Ȃ�����
        if (isIgnored(clazz)) {
            return "";
        }

        // �W�J���Ȃ����̂͂��̂܂�
        if (isNotExpanded(clazz)) {
            return String.valueOf(object);
        }

        // �z���R���N�V�����Ȃ�W�J���� TODO �z��͂܂�...
        if (object instanceof List) {
            return expand((List<?>) object);
        } else if (object instanceof Set) {
            return expand((Set<?>) object);
        } else if (object instanceof Map) {
            return expand((Map<?, ?>) object);
        } else if (object instanceof int[]) {
            return expand((int[]) object);
        } else if (object instanceof byte[]) {
            return expand((byte[]) object);
        } else if (object instanceof Object[]) {
            return expand((Object[]) object);
        }

        StringBuilder sb = new StringBuilder();

        sb.append(clazz.getName());
        sb.append("{");

        // private ���\�b�h�A�t�B�[���h�̎擾�ɂ� getDeclared...
        // ���g���܂��B
        Field[] fields = clazz.getDeclaredFields();
// Debug.println("fialds: " + fields.length);
        boolean isFirst = true;

        for (int i = 0; i < fields.length; i++) {
            Field field = fields[i];

            // private �t�B�[���h�̎擾�ɂ́Aaccessible �t���O��
            // true �ɂ���K�v������܂��B
            field.setAccessible(true);

            String name = field.getName();
// Debug.println("fiald name: " + name);
            Object value = null;
            try {
                if (depth > 0) {
                    if (depth <= recursiveDepth) {
                        value = paramString(field.get(object), depth + 1);
                    } else {
                        value = paramString(field.get(object));
                    }
                } else {
                    value = field.get(object);
                }
            } catch (IllegalAccessException e) {
                value = "*";
            }
// Debug.println("field: " + name + ": type: " + field.getDeclaringClass() + ": " + (value != null ? value.getClass() : null));

            int modifiers = field.getModifiers();
            if (ignoredStatics && Modifier.isStatic(modifiers)) {
                // static �𖳎�
// Debug.println("ignore statics: " + modifiers);
            } else if (ignoredFinals && Modifier.isFinal(modifiers)) {
                // final �𖳎�
// 		if (name.startsWith("this$")) {
                    // static �łȂ� inner class �� outer class �I�u�W�F�N�g
// Debug.println("value: " + value);
// 	        } else {
// Debug.println("ignore final: " + modifiers);
// 	        }
            } else if (ignoredInnerClasses &&
//                       value != null &&
                       (value instanceof Class || value instanceof String) &&
                       name.startsWith("class$")) {
                // inner class �𖳎� TODO class$ �� SUN j2se �Ɉˑ����Ă���
// Debug.println("ignore innerclass: " + value.getClass());
            } else {
                if (isFirst) {
                    isFirst = false;
                } else {
                    sb.append(",");
                }
                sb.append(name);
                sb.append("=");
                // TODO if (formattable) {
                if (value instanceof Calendar) {
                    sb.append(sdf.format(((Calendar) value).getTime()));
                } else if (value  instanceof java.util.Date) {
                    sb.append(sdf.format((Date) value));
                } else {
                    sb.append(value);
                }
            }
        }

        if (!ignoredSuperClass) {
            // �X�[�p�[�N���X��W�J
            Class<?> superClass = clazz.getSuperclass();
            if (superClass != null && !isNotExpanded(superClass)) {
//              String name = getClassName(superClass);
                Object value = null;
                if (depth > 0 && depth <= recursiveDepth + 1) {
                    value = paramString(superClass, object, depth - 1);
                } else {
//Debug.println("depth: " + depth + ", super: " + superClass + ", object: " + object + ", recursiveDepth: " + recursiveDepth);
                    value = paramString(superClass, object, recursiveDepth + 1);
                }

                if (isFirst) {
                    isFirst = false;
                } else {
                    sb.append(",");
                }
                sb.append("super");
                sb.append("=");
                sb.append(value);
            } else {
                // clazz is Object or interface or primitives or void
            }
        }

        sb.append("}");
        return sb.toString();
    }

    /**
     * �W�J���Ȃ��N���X���ǂ����B
     * @see "StringUtil.properties#vavi.util.StringUtil.isNotExpanded.*"
     */
    private static boolean isNotExpanded(Class<?> clazz) {
//Debug.println(clazz + ": " + classesIgnored.contains(clazz.getName()));
        return classesNotExpanded.contains(clazz.getName());
    }

    /**
     * �\�����Ȃ��N���X���ǂ����B
     * @see "StringUtil.properties#vavi.util.StringUtil.isIgnored.*"
     */
    private static boolean isIgnored(Class<?> clazz) {
//Debug.println(clazz + ": " + classesIgnored.contains(clazz.getName()));
        return classesIgnored.contains(clazz.getName());
    }

    /**
     * �p�b�P�[�W������菜�����N���X�����擾���܂��D
     * 
     * @param name �p�b�P�[�W�t���̃N���X��
     * @return �p�b�P�[�W������菜�����N���X��
     */
    public static final String getClassName(String name) {
        return name.substring(name.lastIndexOf(".") + 1);
    }

    /**
     * �p�b�P�[�W������菜�����N���X�����擾���܂��D
     * 
     * @param clazz �N���X
     * @return �p�b�P�[�W������菜�����N���X��
     */
    public static final String getClassName(Class<?> clazz) {
        return getClassName(clazz.getName());
    }

    /**
     * �z���W�J���܂��D
     * TODO deep flag
     */
    public static final String expand(int[] array) {
        Integer[] objectArray = new Integer[array.length];
        for (int i = 0; i < array.length; i++) {
            objectArray[i] = new Integer(array[i]);
        }
        return expand(objectArray);
    }

    /**
     * �z���W�J���܂��D
     * @see #bytesExpand �W�J����ő�l
     * TODO deep flag
     */
    public static final String expand(byte[] array) {
        StringBuilder sb = new StringBuilder();
        sb.append("[");
        sb.append(array.length);
        sb.append("]");
        sb.append("{");
        for (int i = 0; i < Math.min(array.length, bytesExpand); i++) {
            sb.append(getPrintableChar((char) array[i]));
        }
        sb.append("}");
        return sb.toString();
    }

    /**
     * �z���W�J���܂��D
     * TODO deep flag
     */
    public static final String expand(Object[] array) {
        StringBuilder sb = new StringBuilder(array.getClass().getName());
        sb.append("[");
        sb.append(array.length);
        sb.append("]");
        sb.append("{");
        for (int i = 0; i < array.length; i++) {
            sb.append(paramString(array[i], 1));
            sb.append(",");
        }
        if (array.length > 0) {
            sb.setLength(sb.length() - 1);
        }
        sb.append("}");

        return sb.toString();
    }

    /**
     * List ��W�J���܂��D
     * <pre>
     * className[collectionSize]{value0,value1,value2,...}
     * </pre>
     * TODO deep flag
     */
    public static final String expand(List<?> list) {
//Debug.println(list.getClass() + ", " + list.hashCode());
        StringBuilder sb = new StringBuilder(list.getClass().getName());
        sb.append("[");
        sb.append(list.size());
        sb.append("]");
        sb.append("{");
        Iterator<?> i = list.iterator();
        while (i.hasNext()) {
            Object object = i.next();
//Debug.println(object.getClass() + ", " + object.hashCode());
            sb.append(paramString(object));
            sb.append(",");
        }
        if (list.size() > 0) {
            sb.setLength(sb.length() - 1);
        }
        sb.append("}");

        return sb.toString();
    }

    /**
     * Set ��W�J���܂��D
     * <pre>
     * className[collectionSize]{value0,value1,value2,...}
     * </pre>
     * TODO deep flag
     */
    public static final String expand(Set<?> set) {
        StringBuilder sb = new StringBuilder(set.getClass().getName());
        sb.append("[");
        sb.append(set.size());
        sb.append("]");
        sb.append("{");
        Iterator<?> i = set.iterator();
        while (i.hasNext()) {
            Object object = i.next();
            sb.append(paramString(object, 1));
            sb.append(",");
        }
        if (set.size() > 0) {
            sb.setLength(sb.length() - 1);
        }
        sb.append("}");

        return sb.toString();
    }

    /**
     * Map ��W�J���܂��D
     * <pre>
     * className[collectionSize]{key0=value0,key1=value1,key2=value2,...}
     * </pre>
     * TODO deep flag
     */
    public static final String expand(Map<?, ?> map) {
        StringBuilder sb = new StringBuilder(map.getClass().getName());
        sb.append("[");
        sb.append(map.size());
        sb.append("]");
        sb.append("{");
        Iterator<?> i = map.keySet().iterator();
        while (i.hasNext()) {
            Object key = i.next();
            Object value = map.get(key);
            sb.append(paramString(key, 1) + "=" + paramString(value, 1));
            sb.append(",");
        }
        if (map.size() > 0) {
            sb.setLength(sb.length() - 1);
        }
        sb.append("}");

        return sb.toString();
    }

    /**
     * �o�C�g�z��� 16 �i���Ń_���v���܂��D
     */
    public static final String getDump(byte[] buf) {
        return getDump(new ByteArrayInputStream(buf));
    }

    /**
     * ���������t�Ńo�C�g�z��� 16 �i���Ń_���v���܂��D
     */
    public static final String getDump(byte[] buf, int length) {
        return getDump(new ByteArrayInputStream(buf), length);
    }

    /**
     * ���������t�� offset ����̃o�C�g�z��� 16 �i���Ń_���v���܂��D
     */
    public static final String getDump(byte[] buf, int offset, int length) {
        return getDump(new ByteArrayInputStream(buf, offset, length));
    }

    /** */
    private static final char getPrintableChar(char c) {
        return !Character.isISOControl(c) ? c : '.';
    }

    /**
     * �X�g���[���� 16 �i���Ń_���v���܂��D
     */
    public static final String getDump(InputStream is) {

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(baos);

        try {
            if (is.markSupported()) {
                is.mark(is.available());
            }

            byte[] buf = new byte[16];
            boolean breakFlag = false;
            int m = 0;

top:
            while (true) {
                for (int y = 0; y < 16; y++) {
                    for (int x = 0; x < 16; x++) {
                        int c = is.read();
                        if (c == -1) {
                            if (!breakFlag) {
                                breakFlag = true;
                                m = x;
                            }
                            if (m > 0) {
                                ps.print("   ");
                            } else {
                                break;
                            }
                        } else {
                            ps.print(toHex2(c) + " ");
                            buf[x] = (byte) c;
                        }
                    }
                    for (int x = 0; x < 16; x++) {
                        if (breakFlag && x == m) {
                            ps.println();
                            break top;
                        } else {
                            ps.print(getPrintableChar((char) buf[x]));
                        }
                    }
                    ps.println();
                }
                ps.println();
            }
        } catch (IOException e) {
Debug.printStackTrace(e);
        } finally {
            if (is.markSupported()) {
                try {
                    is.reset();
                } catch (IOException e) {
Debug.printStackTrace(e);
                }
            }
        }

        return baos.toString();
    }

    /**
     * ���������t�ŃX�g���[���� 16 �i���Ń_���v���܂��D
     * TODO ���t�@�N�^�����O�H����ˁ[��
     * @param length �������钷��
     */
    public static final String getDump(InputStream is, int length) {

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(baos);

        int count = 0;

        try {
            if (is.markSupported()) {
                is.mark(is.available());
            }

    	    byte[] buf = new byte[16];
    	    boolean breakFlag = false;
    	    int m = 0;

top:	    while (true) {
                for (int y = 0; y < 16; y++) {
                    for (int x = 0; x < 16; x++) {
                        int c = is.read();
                        count++;
                        if (count > length) {
                            ps.println();
                            return baos.toString();
                        }
                        if (c == -1) {
                            if (!breakFlag) {
                                breakFlag = true;
                                m = x;
                            }
                            if (m > 0) {
                                ps.print("   ");
                            } else {
                                break;
                            }
                        } else {
                            ps.print(toHex2(c) + " ");
                            buf[x] = (byte) c;
                        }
                    }
                    for (int x = 0; x < 16; x++) {
                        if (breakFlag && x == m) {
                            ps.println();
                            break top;
                        } else {
                            ps.print(getPrintableChar((char) buf[x]));
                        }
                    }
                    ps.println();
                }
                ps.println();
            }
        } catch (IOException e) {
Debug.printStackTrace(e);
        } finally {
            if (is.markSupported()) {
                try {
                    is.reset();
                } catch (IOException e) {
Debug.printStackTrace(e);
                }
            }
        }

        return baos.toString();
    }

    /**
     * ������� 16 �i���Ń_���v���܂��D
     */
    public static final String getDump(String s) {
        StringBuilder sb = new StringBuilder();
        int l = s.length();
        for (int i = 0; i < l; i++) {
            sb.append(toHex4(s.charAt(i)));
            sb.append(i == l - 1 ? "" : " ");
        }
        return sb.toString();
    }

    /**
     * ������� 16 �i���Ń_���v���܂��D
     */
    public static final String getDump(String s, String encoding) {
        try {
            byte[] b = s.getBytes(encoding);
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < b.length; i++) {
                sb.append(toHex2(b[i]));
                sb.append(i == b.length - 1 ? "" : " ");
            }
            return sb.toString();
        } catch (UnsupportedEncodingException e) {
e.printStackTrace(System.err);
            return getDump(s);
        }
    }

    /**
     * ������� char �Ń_���v���܂��D
     */
    public static final String getCharDump(String s) {
        StringBuilder sb = new StringBuilder();
        int l = s.length();
        for (int i = 0; i < l; i++) {
            char c = s.charAt(i);
            sb.append("   ");
            sb.append(!Character.isISOControl(c) ? c : '.');
            sb.append(i == l - 1 ? "" : " ");
        }
        return sb.toString();
    }

    /**
     * �擪�� 0 �Ŗ��߂� 2 ���̑啶���� 16 �i����Ԃ��܂��D
     */
    public static final String toHex2(int i) {
        String s = "0" + Integer.toHexString(i);
        s = s.substring(s.length() - 2);
        return hexUpperCase ? s.toUpperCase() : s;
    }
    
    /**
     * �擪�� 0 �Ŗ��߂� 4 ���̑啶���� 16 �i����Ԃ��܂��D
     */
    public static final String toHex4(int i) {
        String s = "000" + Integer.toHexString(i);
        s = s.substring(s.length() - 4);
        return hexUpperCase ? s.toUpperCase() : s;
    }
    
    /**
     * �擪�� 0 �Ŗ��߂� 8 ���̑啶���� 16 �i����Ԃ��܂��D
     */
    public static final String toHex8(int i) {
        String s = "0000000" + Integer.toHexString(i);
        s = s.substring(s.length() - 8);
        return hexUpperCase ? s.toUpperCase() : s;
    }
    
    /**
     * �擪�� 0 �Ŗ��߂� 16 ���̑啶���� 16 �i����Ԃ��܂��D
     */
    public static final String toHex16(long l) {
        String s = "000000000000000" + Long.toHexString(l);
        s = s.substring(s.length() - 16);
        return hexUpperCase ? s.toUpperCase() : s;
    }
    
    /**
     * �r�b�g���\���������Ԃ��܂��D
     */
    public static final String toBits(int b) {
	return toBits(b, 8);
    }

    /**
     * �r�b�g���\���������Ԃ��܂��D
     * @param	b	�r�b�g��ɂ��������l
     * @param	n	�r�b�g��̌���
     */
    public static final String toBits(int b, int n) {
        int mask = 0x0001 << (n - 1);
//Debug.println(toHex8(mask));
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < n; i++) {
            sb.append((b & (mask >>> i)) != 0 ? onBit : offBit);
//Debug.println(toHex8(mask >>> i));
        }
	    return sb.toString();
    }

    /**
     * @param s String object to capitalize.
     * @return a capitalized String object.
     */
    public static String capitalize(String s) {
        return s.substring(0, 1).toUpperCase() + s.substring(1);
    }

    /**
     * �R�}���h���C���𕪊����܂��B
     *
     *  @param line  The String to tokenize
     *  @return An array of substrings.
     *  @exception IOException Thrown if StreamTokenizer.nextToken() throws it.
     */
    public static String[] splitCommandLine(String line) throws IOException {

        // The java.lang.Runtime.exec(String command) call uses
        // java.util.StringTokenizer() to parse the command string.
        // Unfortunately, this means that double quotes are not handled
        // in the same way that the shell handles them in that 'ls "foo
        // 'bar"' will interpreted as three tokens 'ls', '"foo' and
        // 'bar"'.  In the shell, the string would be two tokens 'ls' and
        // '"foo bar"'.  What is worse is that the exec() behaviour is
        // slightly different under Windows and Unix.  To solve this
        // problem, we preprocess the command argument using
        // java.io.StreamTokenizer, which converts quoted substrings into
        // single tokens.  We then call java.lang.Runtime.exec(String []
        // commands);

        // Parse the command into tokens
        List<String> commandList = new LinkedList<String>();

        StreamTokenizer streamTokenizer =
            new StreamTokenizer(new StringReader(line));

        // We reset the syntax so that we don't convert to numbers,
        // otherwise, if PTII is "d:\\tmp\\ptII\ 2.0", then
        // we have no end of problems.
        streamTokenizer.resetSyntax();
        streamTokenizer.whitespaceChars(0 , 32);
        streamTokenizer.wordChars(33, 127);

        // We can't use quoteChar here because it does backslash
        // substitution, so "c:\ptII" ends up as "c:ptII"
        // Substituting forward slashes for backward slashes seems like
        // overkill.
        // streamTokenizer.quoteChar('"');
        streamTokenizer.ordinaryChar('"');

        streamTokenizer.eolIsSignificant(true);

        // Current token
        String token = "";

        // Single character token, usually a -
        String singleToken = "";

        // Set to true if we are inside a double quoted String.
        boolean inDoubleQuotedString = false;

        while (streamTokenizer.nextToken() != StreamTokenizer.TT_EOF) {

            switch (streamTokenizer.ttype) {
            case StreamTokenizer.TT_WORD:
            case StreamTokenizer.TT_NUMBER:
                if (inDoubleQuotedString) {
                    if (token.length() > 0) {
                        token += " ";
                    }
                    token += singleToken + streamTokenizer.sval;
                } else {
                    token = singleToken + streamTokenizer.sval;
                    commandList.add(token);
                }
                singleToken = "";
                break;
            case StreamTokenizer.TT_EOL:
                break;
            case StreamTokenizer.TT_EOF:
                if (inDoubleQuotedString) {
                    throw new IllegalArgumentException("unbalanced quote");
                }
                break;
            default:
                singleToken =
                    (new Character((char) streamTokenizer.ttype)).toString();
                if (singleToken.equals("\"")) {
                    if (inDoubleQuotedString) {
                        commandList.add(token);
                    }
                    inDoubleQuotedString = ! inDoubleQuotedString;
                    singleToken = "";
                    token = "";
                }
                break;
            }
        }

        return commandList.toArray(new String[commandList.size()]);
    }

    //----

    /** @see #toBits(int) */
    private static String onBit = "*";

    /** @see #toBits(int) */
    private static String offBit = ".";

    /** @see #toHex2(int) */
    private static boolean hexUpperCase = true;

    /** @see #expand(byte[]) */
    private static int bytesExpand = 16;

    /** @see #paramString(Object) */
    private static int recursiveDepth = 3;

    /** @see #paramString(Object) */
    private static boolean ignoredStatics = false;

    /** @see #paramString(Object) */
    private static boolean ignoredFinals = true;

    /** @see #paramString(Object) */
    private static boolean ignoredInnerClasses = true;

    /** @see #paramString(Object) */
    private static boolean ignoredSuperClass = false;

    /** isNotExpanded �̃N���X�W */
    private static Set<String> classesNotExpanded = new HashSet<String>();

    /** isIgnored �̃N���X�W */
    private static Set<String> classesIgnored = new HashSet<String>();

    /** */
    static {
        try {
            Properties props = new Properties();
            final String path = "StringUtil.properties";
            props.load(StringUtil.class.getResourceAsStream(path));

            // toBits
            onBit = props.getProperty("vavi.util.StringUtil.string.onBit");
            offBit = props.getProperty("vavi.util.StringUtil.string.offBit");

            // isNotExpanded
            Iterator<?> i = props.keySet().iterator();
            while (i.hasNext()) {
                String key = (String) i.next();
                if (key.startsWith("vavi.util.StringUtil.isNotExpanded.")) {
                    String className = props.getProperty(key);
                    classesNotExpanded.add(className);
//Debug.println(classesNotExpanded.size() + ", class: " + className);
                }
            }

            // isIgnored
            i = props.keySet().iterator();
            while (i.hasNext()) {
                String key = (String) i.next();
                if (key.startsWith("vavi.util.StringUtil.isIgnored.")) {
                    String className = props.getProperty(key);
                    classesNotExpanded.add(className);
//Debug.println(classesIgnored.size() + ", class: " + className);
                }
            }

            String value = null;

            // recursive depth
            value = props.getProperty("vavi.util.StringUtil.recursiveDepth");
            recursiveDepth = new Integer(value).intValue();
//Debug.println("recursiveDepth: " + recursiveDepth);

            // bytes expand
            value = props.getProperty("vavi.util.StringUtil.bytesExpand");
            bytesExpand = new Integer(value).intValue();
//Debug.println("bytesExpand: " + bytesExpand);

            // ignore static
            value = props.getProperty("vavi.util.StringUtil.isIgnored.static");
            ignoredStatics = new Boolean(value).booleanValue();
//Debug.println("ignoredStatics: " + ignoredStatics);

            // ignore final
            value = props.getProperty("vavi.util.StringUtil.isIgnored.final");
            ignoredFinals = new Boolean(value).booleanValue();
//Debug.println("ignoredFinals: " + ignoredFinals);

            // ignore inner class
            value = props.getProperty("vavi.util.StringUtil.isIgnored.inner");
            ignoredInnerClasses = new Boolean(value).booleanValue();
//Debug.println("ignoredInnerClasses: " + ignoredInnerClasses);

            // ignore super class
            value = props.getProperty("vavi.util.StringUtil.isIgnored.super");
            ignoredSuperClass = new Boolean(value).booleanValue();
//Debug.println("ignoredSuperClass: " + ignoredSuperClass);

            // hex case upper
            value = props.getProperty("vavi.util.StringUtil.toHex.upperCase");
            hexUpperCase = new Boolean(value).booleanValue();
//Debug.println("hexUpperCase: " + hexUpperCase);
        } catch (Exception e) {
Debug.printStackTrace(e);
        }
    }

    //----

    /** */
    public static void main(String[] args) throws IOException {
/*
        System.err.println("---- getDump(String) ----");
        System.err.println(getDump("���쒼�G"));
        System.err.println("---- getDump(InputStream) ----");
        System.err.println(getDump(new FileInputStream("StringUtil.class")));
*/
        System.err.println("---- toBits(int,int) ----");
        System.err.println(toBits(0x842173c6, 32));
        System.err.println("---- toBits(int,int) ----");
        System.err.println(toBits(0x73c6, 16));
        System.err.println("---- toBits(int,int) ----");
        System.err.println(toBits(0xc6, 8));
    }
}

/* */
