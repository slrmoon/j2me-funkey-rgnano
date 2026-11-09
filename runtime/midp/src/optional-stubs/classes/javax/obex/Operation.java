package javax.obex;

import java.io.IOException;

import javax.microedition.io.ContentConnection;

public interface Operation extends ContentConnection {
    void abort() throws IOException;

    HeaderSet getReceivedHeaders() throws IOException;

    void sendHeaders(HeaderSet headers) throws IOException;

    int getResponseCode() throws IOException;
}
