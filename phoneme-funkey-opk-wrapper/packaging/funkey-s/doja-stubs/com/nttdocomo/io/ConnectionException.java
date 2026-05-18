package com.nttdocomo.io;

import java.io.IOException;

public class ConnectionException extends IOException {
    private int status;

    public ConnectionException() {
    }

    public ConnectionException(String message) {
        super(message);
    }

    public ConnectionException(int status) {
        this.status = status;
    }

    public ConnectionException(int status, String message) {
        super(message);
        this.status = status;
    }

    public int getStatus() {
        return status;
    }
}
