/* stack growth and exceptions through compiled frames, in several threads */
public class StackTest implements Runnable {
    static volatile boolean alive = true;
    static int errors, rounds;
    int id;
    StackTest(int i) { id = i; }

    static int depth(int n, int acc, Object o) {
        if (n == 0) return acc + (o == null ? 0 : 1);
        int[] pad = (n & 15) == 0 ? new int[8] : null;
        int r = depth(n - 1, acc + n, pad == null ? o : (Object) pad);
        return r - (pad == null ? 0 : pad.length - 8);
    }
    static long ldepth(int n, long a, double d) {
        if (n == 0) return a + (long) d;
        return ldepth(n - 1, a + n, d + 0.5) + 1;
    }
    static int thrower(int n, int at) {
        if (n == at) throw new IllegalArgumentException("t" + n);
        try {
            return thrower(n + 1, at) + 1;
        } finally {
            rounds++;
        }
    }
    synchronized int locked(int n) {
        if (n == 0) return id;
        return locked(n - 1) + 1;
    }
    public void run() {
        int k = 0;
        while (alive) {
            int n = 10 + (k * 37 + id * 11) % 900;
            int want = n * (n + 1) / 2 + 1;
            int got = depth(n, 0, this);
            if (got != want) { errors++; System.out.println("FAIL depth " + n + " " + got + " " + want); }
            long lw = (long) n * (n + 1) / 2 + (long) (n * 0.5) + n;
            long lg = ldepth(n, 0L, 0.0);
            if (lg != lw) { errors++; System.out.println("FAIL ldepth " + n + " " + lg + " " + lw); }
            int at = 5 + k % 60;
            try { thrower(0, at); errors++; System.out.println("FAIL no throw"); }
            catch (IllegalArgumentException e) { if (!e.getMessage().equals("t" + at)) { errors++; System.out.println("FAIL msg"); } }
            if (locked(n % 200) != id + n % 200) { errors++; System.out.println("FAIL locked"); }
            k++;
            if ((k & 31) == 0) Thread.yield();
        }
    }
    public static void main(String[] args) {
        int secs = args.length > 0 ? Integer.parseInt(args[0]) : 10;
        for (int i = 0; i < 3; i++) new Thread(new StackTest(i)).start();
        long end = System.currentTimeMillis() + secs * 1000L;
        int spawned = 0;
        while (System.currentTimeMillis() < end && errors == 0) {
            final int base = spawned++;
            Thread t = new Thread(new Runnable() { public void run() {
                int n = 300 + base % 500;
                if (depth(n, 0, null) != n * (n + 1) / 2 + 1) { errors++; System.out.println("FAIL new thread depth"); }
            } });
            t.start();
            try { Thread.sleep(2); } catch (InterruptedException e) { }
        }
        alive = false;
        try { Thread.sleep(300); } catch (InterruptedException e) { }
        System.out.println("done spawned=" + spawned + " rounds=" + rounds + " errors=" + errors);
    }
}
