package javax.obex;

import java.io.IOException;

public interface HeaderSet {
    int COUNT = 0xC0;
    int NAME = 0x01;
    int TYPE = 0x42;
    int LENGTH = 0xC3;
    int TIME_ISO_8601 = 0x44;
    int TIME_4_BYTE = 0xC4;
    int DESCRIPTION = 0x05;
    int TARGET = 0x46;
    int HTTP = 0x47;
    int WHO = 0x4A;
    int OBJECT_CLASS = 0x4F;
    int APPLICATION_PARAMETER = 0x4C;

    void setHeader(int headerID, Object headerValue);

    Object getHeader(int headerID) throws IOException;

    int[] getHeaderList() throws IOException;

    void createAuthenticationChallenge(String realm, boolean userID, boolean access);

    int getResponseCode() throws IOException;
}
