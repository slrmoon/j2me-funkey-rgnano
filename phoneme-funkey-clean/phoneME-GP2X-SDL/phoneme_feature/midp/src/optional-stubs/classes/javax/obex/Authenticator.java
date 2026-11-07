package javax.obex;

public interface Authenticator {
    PasswordAuthentication onAuthenticationChallenge(String description, boolean isUserIdRequired,
            boolean isFullAccess);

    byte[] onAuthenticationResponse(byte[] userName);
}
