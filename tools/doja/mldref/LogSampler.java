import opendoja.audio.mld.*;
public class LogSampler implements Sampler, SamplerProvider {
  final Sampler s; long pos; final java.io.PrintStream o;
  public LogSampler(Sampler s, java.io.PrintStream o) { this.s = s; this.o = o; }
  public Sampler instance(float r) { return this; }
  void L(String x) { o.println(pos + " " + x); }
  public void bankChange(int c, int b) { L("bank " + c + " " + b); s.bankChange(c, b); }
  public void drumEnable(int c, boolean e) { L("drum " + c + " " + e); s.drumEnable(c, e); }
  public boolean isFinished() { return s.isFinished(); }
  public MLDPlaybackEngine createPlaybackEngine(MLD m, float r) { return s.createPlaybackEngine(m, r); }
  public void keyOff(int c, int k) { L("off " + c + " " + k); s.keyOff(c, k); }
  public void keyOn(int c, int k, float v) { L("on " + c + " " + k + " " + v); s.keyOn(c, k, v); }
  public void masterTune(float x) { L("mtune " + x); s.masterTune(x); }
  public void masterVolume(float x) { L("mvol " + x); s.masterVolume(x); }
  public void panpot(int c, float x) { L("pan " + c + " " + x); s.panpot(c, x); }
  public void pitchBend(int c, float x) { L("bend " + c + " " + x); s.pitchBend(c, x); }
  public void pitchBendRange(int c, float x) { L("range " + c + " " + x); s.pitchBendRange(c, x); }
  public void programChange(int c, int p) { L("prog " + c + " " + p); s.programChange(c, p); }
  public void render(float[] a, int b, int f) { s.render(a, b, f); pos += f; }
  public void render(float[] a, int b, int f, float g) { s.render(a, b, f, g); pos += f; }
  public void render(float[] a, int b, int f, float l, float r) { s.render(a, b, f, l, r); pos += f; }
  public void render(float[] a, int b, int f, float l, float r, boolean e, boolean c) { s.render(a, b, f, l, r, e, c); pos += f; }
  public void reset() { s.reset(); }
  public float sampleRate() { return s.sampleRate(); }
  public void stopAll() { L("stopall"); s.stopAll(); }
  public void sysEx(byte[] m) { L("sysex " + m.length); s.sysEx(m); }
  public boolean suppressActiveKeyRetrigger() { return s.suppressActiveKeyRetrigger(); }
  public void volume(int c, float x) { L("vol " + c + " " + x); s.volume(c, x); }
}
