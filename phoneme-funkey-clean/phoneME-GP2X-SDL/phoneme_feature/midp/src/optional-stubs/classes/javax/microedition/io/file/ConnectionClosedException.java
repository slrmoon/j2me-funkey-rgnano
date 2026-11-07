package javax.microedition.io.file;

public class ConnectionClosedException extends RuntimeException {
    public ConnectionClosedException() {
    }

    public ConnectionClosedException(String detailMessage) {
        super(detailMessage);
    }
}
