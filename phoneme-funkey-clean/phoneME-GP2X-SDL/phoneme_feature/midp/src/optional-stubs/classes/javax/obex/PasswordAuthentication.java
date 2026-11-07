package javax.obex;

public class PasswordAuthentication {
    private final byte[] userName;
    private final byte[] password;

    public PasswordAuthentication(byte[] userName, byte[] password) {
        if (password == null) {
            throw new NullPointerException();
        }
        this.userName = userName;
        this.password = password;
    }

    public byte[] getUserName() {
        return userName;
    }

    public byte[] getPassword() {
        return password;
    }
}
