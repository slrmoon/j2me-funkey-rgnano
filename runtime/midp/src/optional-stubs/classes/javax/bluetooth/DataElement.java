package javax.bluetooth;

import java.util.Enumeration;
import java.util.Vector;

/** SDP data element: value storage as the spec describes. */
public class DataElement {
    public static final int NULL = 0x0000;
    public static final int U_INT_1 = 0x0008;
    public static final int U_INT_2 = 0x0009;
    public static final int U_INT_4 = 0x000A;
    public static final int U_INT_8 = 0x000B;
    public static final int U_INT_16 = 0x000C;
    public static final int INT_1 = 0x0010;
    public static final int INT_2 = 0x0011;
    public static final int INT_4 = 0x0012;
    public static final int INT_8 = 0x0013;
    public static final int INT_16 = 0x0014;
    public static final int URL = 0x0040;
    public static final int UUID = 0x0018;
    public static final int BOOL = 0x0028;
    public static final int STRING = 0x0020;
    public static final int DATSEQ = 0x0030;
    public static final int DATALT = 0x0038;

    private final int type;
    private long longValue;
    private boolean boolValue;
    private Object value;

    public DataElement(int valueType) {
        if (valueType != NULL && valueType != DATSEQ && valueType != DATALT) {
            throw new IllegalArgumentException();
        }
        type = valueType;
        if (valueType != NULL) {
            value = new Vector();
        }
    }

    public DataElement(boolean bool) {
        type = BOOL;
        boolValue = bool;
    }

    public DataElement(int valueType, long value) {
        switch (valueType) {
        case U_INT_1: case U_INT_2: case U_INT_4: case INT_1: case INT_2: case INT_4: case INT_8:
            break;
        default:
            throw new IllegalArgumentException();
        }
        type = valueType;
        longValue = value;
    }

    public DataElement(int valueType, Object value) {
        if (value == null) {
            throw new IllegalArgumentException();
        }
        switch (valueType) {
        case URL: case STRING: case UUID: case U_INT_8: case U_INT_16: case INT_16:
            break;
        default:
            throw new IllegalArgumentException();
        }
        type = valueType;
        this.value = value;
    }

    private Vector seq() {
        if (type != DATSEQ && type != DATALT) {
            throw new ClassCastException();
        }
        return (Vector) value;
    }

    public void addElement(DataElement elem) {
        if (elem == null) {
            throw new NullPointerException();
        }
        seq().addElement(elem);
    }

    public void insertElementAt(DataElement elem, int index) {
        if (elem == null) {
            throw new NullPointerException();
        }
        seq().insertElementAt(elem, index);
    }

    public int getSize() {
        return seq().size();
    }

    public boolean removeElement(DataElement elem) {
        if (elem == null) {
            throw new NullPointerException();
        }
        return seq().removeElement(elem);
    }

    public int getDataType() {
        return type;
    }

    public long getLong() {
        switch (type) {
        case U_INT_1: case U_INT_2: case U_INT_4: case INT_1: case INT_2: case INT_4: case INT_8:
            return longValue;
        default:
            throw new ClassCastException();
        }
    }

    public boolean getBoolean() {
        if (type != BOOL) {
            throw new ClassCastException();
        }
        return boolValue;
    }

    public Object getValue() {
        switch (type) {
        case URL: case STRING: case UUID: case U_INT_8: case U_INT_16: case INT_16:
            return value;
        case DATSEQ: case DATALT:
            return ((Vector) value).elements();
        default:
            throw new ClassCastException();
        }
    }
}
