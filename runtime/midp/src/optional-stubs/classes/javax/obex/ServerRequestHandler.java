package javax.obex;

public class ServerRequestHandler {
    private long connectionID = -1;

    protected ServerRequestHandler() {
    }

    public final HeaderSet createHeaderSet() {
        return null;
    }

    public void setConnectionID(long id) {
        if (id < -1 || id > 0xffffffffL) {
            throw new IllegalArgumentException();
        }
        connectionID = id;
    }

    public long getConnectionID() {
        return connectionID;
    }

    public int onConnect(HeaderSet request, HeaderSet reply) {
        return ResponseCodes.OBEX_HTTP_OK;
    }

    public void onDisconnect(HeaderSet request, HeaderSet reply) {
    }

    public int onSetPath(HeaderSet request, HeaderSet reply, boolean backup, boolean create) {
        return ResponseCodes.OBEX_HTTP_NOT_IMPLEMENTED;
    }

    public int onDelete(HeaderSet request, HeaderSet reply) {
        return ResponseCodes.OBEX_HTTP_NOT_IMPLEMENTED;
    }

    public int onPut(Operation op) {
        return ResponseCodes.OBEX_HTTP_NOT_IMPLEMENTED;
    }

    public int onGet(Operation op) {
        return ResponseCodes.OBEX_HTTP_NOT_IMPLEMENTED;
    }

    public void onAuthenticationFailure(byte[] userName) {
    }
}
