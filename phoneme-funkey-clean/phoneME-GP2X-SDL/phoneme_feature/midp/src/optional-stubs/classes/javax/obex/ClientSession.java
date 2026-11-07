package javax.obex;

import java.io.IOException;

import javax.microedition.io.Connection;

public interface ClientSession extends Connection {
    void setAuthenticator(Authenticator auth);

    HeaderSet createHeaderSet();

    void setConnectionID(long id);

    long getConnectionID();

    HeaderSet connect(HeaderSet headers) throws IOException;

    HeaderSet disconnect(HeaderSet headers) throws IOException;

    HeaderSet setPath(HeaderSet headers, boolean backup, boolean create) throws IOException;

    HeaderSet delete(HeaderSet headers) throws IOException;

    Operation get(HeaderSet headers) throws IOException;

    Operation put(HeaderSet headers) throws IOException;
}
