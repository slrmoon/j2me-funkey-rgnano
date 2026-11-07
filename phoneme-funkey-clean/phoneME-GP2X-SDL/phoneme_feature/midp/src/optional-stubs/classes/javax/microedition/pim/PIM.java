package javax.microedition.pim;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;

/**
 * JSR 75 PIM. FunKey: there is no address book or calendar: no list
 * names, and opening a list throws PIMException.
 */
public abstract class PIM {
    public static final int CONTACT_LIST = 1;
    public static final int EVENT_LIST = 2;
    public static final int TODO_LIST = 3;
    public static final int READ_ONLY = 1;
    public static final int WRITE_ONLY = 2;
    public static final int READ_WRITE = 3;

    private static PIM instance;

    protected PIM() {
    }

    public static synchronized PIM getInstance() {
        if (instance == null) {
            instance = new PIM() {
                private void check(int type, int mode) {
                    if (type < CONTACT_LIST || type > TODO_LIST || mode < READ_ONLY || mode > READ_WRITE) {
                        throw new IllegalArgumentException();
                    }
                }

                public PIMList openPIMList(int pimListType, int mode) throws PIMException {
                    check(pimListType, mode);
                    throw new PIMException("PIM not available", PIMException.FEATURE_NOT_SUPPORTED);
                }

                public PIMList openPIMList(int pimListType, int mode, String name) throws PIMException {
                    check(pimListType, mode);
                    if (name == null) {
                        throw new NullPointerException();
                    }
                    throw new PIMException("PIM not available", PIMException.FEATURE_NOT_SUPPORTED);
                }

                public String[] listPIMLists(int pimListType) {
                    if (pimListType < CONTACT_LIST || pimListType > TODO_LIST) {
                        throw new IllegalArgumentException();
                    }
                    return new String[0];
                }

                public PIMItem[] fromSerialFormat(InputStream is, String enc) throws PIMException {
                    if (is == null) {
                        throw new NullPointerException();
                    }
                    throw new PIMException("PIM not available", PIMException.FEATURE_NOT_SUPPORTED);
                }

                public void toSerialFormat(PIMItem item, OutputStream os, String enc,
                        String dataFormat) throws PIMException {
                    if (item == null || os == null || dataFormat == null) {
                        throw new NullPointerException();
                    }
                    throw new PIMException("PIM not available", PIMException.FEATURE_NOT_SUPPORTED);
                }

                public String[] supportedSerialFormats(int pimListType) {
                    if (pimListType < CONTACT_LIST || pimListType > TODO_LIST) {
                        throw new IllegalArgumentException();
                    }
                    return new String[0];
                }
            };
        }
        return instance;
    }

    public abstract PIMList openPIMList(int pimListType, int mode) throws PIMException;

    public abstract PIMList openPIMList(int pimListType, int mode, String name) throws PIMException;

    public abstract String[] listPIMLists(int pimListType);

    public abstract PIMItem[] fromSerialFormat(InputStream is, String enc)
            throws PIMException, UnsupportedEncodingException;

    public abstract void toSerialFormat(PIMItem item, OutputStream os, String enc, String dataFormat)
            throws PIMException, UnsupportedEncodingException;

    public abstract String[] supportedSerialFormats(int pimListType);
}
