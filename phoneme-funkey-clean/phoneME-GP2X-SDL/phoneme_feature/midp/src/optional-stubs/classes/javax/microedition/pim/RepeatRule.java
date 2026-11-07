package javax.microedition.pim;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

/** Field storage only (no PIM lists to repeat events in). */
public class RepeatRule {
    public static final int FREQUENCY = 0;
    public static final int DAY_IN_MONTH = 1;
    public static final int DAY_IN_WEEK = 2;
    public static final int DAY_IN_YEAR = 4;
    public static final int MONTH_IN_YEAR = 8;
    public static final int WEEK_IN_MONTH = 16;
    public static final int COUNT = 32;
    public static final int END = 64;
    public static final int INTERVAL = 128;
    public static final int DAILY = 0x10;
    public static final int WEEKLY = 0x11;
    public static final int MONTHLY = 0x12;
    public static final int YEARLY = 0x13;
    public static final int FIRST = 0x1;
    public static final int SECOND = 0x2;
    public static final int THIRD = 0x4;
    public static final int FOURTH = 0x8;
    public static final int FIFTH = 0x10;
    public static final int LAST = 0x20;
    public static final int SECONDLAST = 0x40;
    public static final int THIRDLAST = 0x80;
    public static final int FOURTHLAST = 0x100;
    public static final int FIFTHLAST = 0x200;
    public static final int SATURDAY = 0x400;
    public static final int FRIDAY = 0x800;
    public static final int THURSDAY = 0x1000;
    public static final int WEDNESDAY = 0x2000;
    public static final int TUESDAY = 0x4000;
    public static final int MONDAY = 0x8000;
    public static final int SUNDAY = 0x10000;
    public static final int JANUARY = 0x20000;
    public static final int FEBRUARY = 0x40000;
    public static final int MARCH = 0x80000;
    public static final int APRIL = 0x100000;
    public static final int MAY = 0x200000;
    public static final int JUNE = 0x400000;
    public static final int JULY = 0x800000;
    public static final int AUGUST = 0x1000000;
    public static final int SEPTEMBER = 0x2000000;
    public static final int OCTOBER = 0x4000000;
    public static final int NOVEMBER = 0x8000000;
    public static final int DECEMBER = 0x10000000;

    private final Hashtable fields = new Hashtable();
    private final Vector except = new Vector();

    public RepeatRule() {
    }

    public Enumeration dates(long startDate, long subsetBeginning, long subsetEnding) {
        Vector v = new Vector();
        if (startDate >= subsetBeginning && startDate <= subsetEnding) {
            v.addElement(new java.util.Date(startDate));
        }
        return v.elements();
    }

    public void addExceptDate(long date) {
        except.addElement(new java.util.Date(date));
    }

    public void removeExceptDate(long date) {
        except.removeElement(new java.util.Date(date));
    }

    public Enumeration getExceptDates() {
        return except.elements();
    }

    public int getInt(int field) {
        Object v = fields.get(new Integer(field));
        if (!(v instanceof Integer)) {
            throw new FieldEmptyException();
        }
        return ((Integer) v).intValue();
    }

    public void setInt(int field, int value) {
        fields.put(new Integer(field), new Integer(value));
    }

    public long getDate(int field) {
        Object v = fields.get(new Integer(field));
        if (!(v instanceof java.util.Date)) {
            throw new FieldEmptyException();
        }
        return ((java.util.Date) v).getTime();
    }

    public void setDate(int field, long value) {
        fields.put(new Integer(field), new java.util.Date(value));
    }

    public int[] getFields() {
        int[] r = new int[fields.size()];
        int i = 0;
        for (Enumeration e = fields.keys(); e.hasMoreElements();) {
            r[i++] = ((Integer) e.nextElement()).intValue();
        }
        return r;
    }

    public boolean equals(Object obj) {
        if (!(obj instanceof RepeatRule)) {
            return false;
        }
        RepeatRule o = (RepeatRule) obj;
        if (o.fields.size() != fields.size()) {
            return false;
        }
        for (Enumeration e = fields.keys(); e.hasMoreElements();) {
            Object k = e.nextElement();
            if (!fields.get(k).equals(o.fields.get(k))) {
                return false;
            }
        }
        return true;
    }

    public int hashCode() {
        return fields.size();
    }
}
