import java.util.Vector;

public class StreamTest implements Runnable {
    static volatile boolean alive = true;
    static volatile int fails = 0;
    static final int N = 64;
    static T[] tiles = new T[N];
    static boolean[] requested = new boolean[N];
    static final int[] reqQ = new int[16];
    static int reqN;
    static final T[] readyQ = new T[8];
    static int readyN;
    static int installed, loads, churn, yields;
    int kind;
    StreamTest(int k) { kind = k; }

    static final class T {
        int id; int[] a; short[] b; int[][] pix; String name; T next; Object[] refs; long sum;
        static T load(int id, int salt) throws Exception {
            if (((id * 31 + salt) & 63) == 7) throw new Exception("x" + id);
            T t = new T();
            t.id = id;
            int n = 20 + (id * 37 + salt) % 300;
            t.a = new int[n];
            for (int i = 0; i < n; i++) t.a[i] = mix(id, i);
            t.b = new short[n / 2 + 1];
            for (int i = 0; i < t.b.length; i++) t.b[i] = (short) mix(i, id);
            int np = 1 + id % 5;
            t.pix = new int[np][];
            for (int k = 0; k < np; k++) t.pix[k] = dec(id, k);
            t.name = "/t/" + id + "_" + n + ".bin";
            t.refs = new Object[4];
            t.refs[0] = t.a; t.refs[1] = t.name; t.refs[2] = t.pix; t.refs[3] = new Integer(id);
            t.sum = total(t.a, 0L) + (long) n * id;
            return t;
        }
        static int[] dec(int id, int k) {
            int[] p = new int[16 + k * 8];
            for (int i = 0; i < p.length; i++) { int c = mix(id + k, i); p[i] = ((c >>> 24) < 128) ? 0 : (c | 0xFF000000); }
            return p;
        }
        static long total(int[] a, long s) { for (int i = 0; i < a.length; i++) s += a[i]; return s; }
        static int mix(int x, int y) { int h = x * 0x9E3779B1 + y * 0x85EBCA6B; h ^= h >>> 13; h *= 0xC2B2AE35; return h ^ (h >>> 16); }
        boolean check() {
            if (a == null || b == null || pix == null || name == null || refs == null) return false;
            for (int i = 0; i < a.length; i++) if (a[i] != mix(id, i)) return false;
            for (int i = 0; i < b.length; i++) if (b[i] != (short) mix(i, id)) return false;
            if (pix.length != 1 + id % 5) return false;
            for (int k = 0; k < pix.length; k++) {
                int[] p = pix[k];
                if (p.length != 16 + k * 8) return false;
                for (int i = 0; i < p.length; i += 5) { int c = mix(id + k, i); if (p[i] != (((c >>> 24) < 128) ? 0 : (c | 0xFF000000))) return false; }
            }
            if (refs[0] != a || refs[1] != name || refs[2] != pix) return false;
            if (((Integer) refs[3]).intValue() != id) return false;
            if (!name.startsWith("/t/" + id + "_")) return false;
            return sum == total(a, 0L) + (long) a.length * id;
        }
    }

    public void run() {
        if (kind == 0) loader();
        else if (kind == 1) churner();
        else yielder();
    }

    void loader() {
        int salt = 0;
        while (alive) {
            int k = -1;
            synchronized (reqQ) {
                if (reqN == 0) { try { reqQ.wait(3); } catch (InterruptedException e) { } }
                if (reqN > 0) { k = reqQ[0]; System.arraycopy(reqQ, 1, reqQ, 0, --reqN); }
            }
            if (k < 0) continue;
            T t;
            try { t = T.load(k, salt++); } catch (Throwable e) { t = null; }
            synchronized (readyQ) {
                if (t == null) requested[k] = false;
                else if (readyN < readyQ.length) readyQ[readyN++] = t;
                else requested[k] = false;
            }
            loads++;
            if ((salt & 7) == 0) { try { Thread.sleep(1); } catch (InterruptedException e) { } }
        }
    }

    void churner() {
        Vector keep = new Vector();
        int r = 12345;
        while (alive) {
            r = r * 1103515245 + 12345;
            int n = 16 + ((r >>> 8) & 1023);
            Object o = ((r & 1) == 0) ? (Object) new int[n] : (Object) new StringBuffer().append(n).append('/').append(r).toString();
            keep.addElement(o);
            if (keep.size() > 40) keep.removeElementAt(((r >>> 4) & 0x7fff) % keep.size());
            churn++;
            if ((churn & 255) == 0) Thread.yield();
        }
    }

    void yielder() {
        T prev = null;
        int i = 0;
        do {
            Thread.yield();
            T t = tiles[i++ & (N - 1)];
            if (t != null && t != prev) { if (!t.check()) fail("yielder " + t.id); prev = t; }
            yields++;
        } while (alive);
    }

    static synchronized void fail(String s) { fails++; System.out.println("FAIL " + s); }

    static void stream(int c) {
        synchronized (readyQ) {
            for (int n = 0; n < readyN; n++) {
                T t = readyQ[n];
                int k = t.id;
                requested[k] = false;
                if (!t.check()) fail("ready " + k);
                if (tiles[k] == null) { tiles[k] = t; t.next = tiles[(k + 1) & (N - 1)]; installed++; }
                readyQ[n] = null;
            }
            readyN = 0;
        }
        for (int ring = 0; ring <= 2; ring++) {
            for (int j = c - ring; j <= c + ring; j++) {
                int k = j & (N - 1);
                if (tiles[k] != null || requested[k]) continue;
                synchronized (reqQ) { if (reqN < reqQ.length) { reqQ[reqN++] = k; requested[k] = true; reqQ.notify(); } }
            }
        }
        for (int k = 0; k < N; k++) {
            T t = tiles[k];
            if (t == null) continue;
            int d = (k - c) & (N - 1); if (d > N / 2) d = N - d;
            if (d > 3) { tiles[k] = null; }
            else if (!t.check()) fail("tile " + k);
        }
    }

    public static void main(String[] args) {
        int secs = args.length > 0 ? Integer.parseInt(args[0]) : 20;
        new Thread(new StreamTest(0)).start();
        new Thread(new StreamTest(1)).start();
        new Thread(new StreamTest(2)).start();
        new Thread(new StreamTest(0)).start();
        long end = System.currentTimeMillis() + secs * 1000L;
        int c = 0, frame = 0;
        while (System.currentTimeMillis() < end && fails == 0) {
            stream(c >> 3);
            c++; frame++;
            if ((frame & 15) == 0) { try { Thread.sleep(1); } catch (InterruptedException e) { } }
        }
        alive = false;
        System.out.println("done frames=" + frame + " installed=" + installed + " loads=" + loads + " churn=" + churn + " yields=" + yields + " fails=" + fails);
    }
}
