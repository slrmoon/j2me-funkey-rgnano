package javax.microedition.io.file;

import java.util.Enumeration;
import java.util.Vector;

/** FunKey: no file system roots for games. */
public class FileSystemRegistry {
    private FileSystemRegistry() {
    }

    public static boolean addFileSystemListener(FileSystemListener listener) {
        if (listener == null) {
            throw new NullPointerException();
        }
        return false;
    }

    public static boolean removeFileSystemListener(FileSystemListener listener) {
        if (listener == null) {
            throw new NullPointerException();
        }
        return false;
    }

    public static Enumeration listRoots() {
        return new Vector().elements();
    }
}
