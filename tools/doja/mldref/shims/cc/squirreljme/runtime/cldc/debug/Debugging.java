package cc.squirreljme.runtime.cldc.debug;
public final class Debugging {
  public static Error oops(Object... a) { return new Error("oops " + java.util.Arrays.toString(a)); }
  public static void todoNote(String f, Object... a) { }
  public static Error todo(Object... a) { return new Error("todo"); }
}
