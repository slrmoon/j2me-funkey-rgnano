import com.keitaiwiki.music.*;
import cc.squirreljme.runtime.media.control.MetaDataValues;
import java.io.*;
import java.nio.*;
import java.nio.file.*;
public class Render {
  public static void main(String[] a) throws Exception {
    byte[] d = Files.readAllBytes(Paths.get(a[0]));
    float rate = a.length > 2 ? Float.parseFloat(a[2]) : 22050f;
    double maxSec = a.length > 3 ? Double.parseDouble(a[3]) : 30;
    MLD mld = new MLD(d, new MetaDataValues());
    MLDPlayer p = new MLDPlayer(mld, new MA3SamplerProvider(), rate);
    p.setLoopEnabled(false);
    int max = (int)(rate * maxSec);
    float[] buf = new float[2048 * 2];
    ByteArrayOutputStream pcm = new ByteArrayOutputStream();
    int total = 0;
    while (total < max) {
      java.util.Arrays.fill(buf, 0);
      int n = p.render(buf, 0, 2048);
      if (n < 0) break;
      for (int i = 0; i < n * 2; i++) {
        int s = Math.round(Math.max(-1f, Math.min(1f, buf[i])) * 32767f);
        pcm.write(s & 255); pcm.write((s >> 8) & 255);
      }
      total += n;
      if (n < 2048) break;
    }
    byte[] data = pcm.toByteArray();
    ByteBuffer h = ByteBuffer.allocate(44).order(ByteOrder.LITTLE_ENDIAN);
    h.put("RIFF".getBytes()).putInt(36 + data.length).put("WAVEfmt ".getBytes()).putInt(16)
     .putShort((short)1).putShort((short)2).putInt((int)rate).putInt((int)rate*4).putShort((short)4).putShort((short)16)
     .put("data".getBytes()).putInt(data.length);
    try (FileOutputStream o = new FileOutputStream(a[1])) { o.write(h.array()); o.write(data); }
    System.out.println(a[0] + " -> " + total + " frames, dur=" + mld.getDuration(true));
  }
}
