/* contended synchronized methods + threads that start all the time */
public class LockTest implements Runnable {
    static volatile boolean alive = true;
    static int[] slots = new int[512];
    static String[] names = new String[512];
    static int spawned, finished, acquires, errors;
    int kind, id;
    LockTest(int k, int i) { kind = k; id = i; }

    static synchronized int find(String path) {
        for (int i = 0; i < names.length; i++) if (path.equals(names[i])) return i;
        return -1;
    }
    static synchronized int acquire(String path, int v) {
        int s = find(path);
        if (s < 0) {
            s = (path.hashCode() & 0x7fffffff) % names.length;
            names[s] = path; slots[s] = 0;
        }
        slots[s] += v;
        acquires++;
        return s;
    }
    synchronized int work(int n) {
        int h = id;
        for (int i = 0; i < n; i++) h = h * 31 + i;
        return h;
    }
    public void run() {
        if (kind == 0) {
            int n = 0;
            while (alive) {
                String p = "/x/" + ((id * 131 + n++) % 700) + ".png";
                int s = acquire(p, 1);
                if (s < 0 || s >= names.length) { errors++; System.out.println("FAIL slot " + s); }
                if (find(names[s]) != s) { /* replaced by another thread: fine */ }
                if (work(50) != work(50)) { errors++; System.out.println("FAIL work"); }
            }
        } else if (kind == 1) {
            while (alive) {
                Thread t = new Thread(new LockTest(2, spawned++));
                t.start();
                try { Thread.sleep(1); } catch (InterruptedException e) { }
            }
        } else {
            int a = acquire("/s/" + (id & 63), 2);
            int h = work(200);
            if (a < 0 || h != work(200)) { errors++; System.out.println("FAIL short"); }
            finished++;
        }
    }
    public static void main(String[] args) {
        int secs = args.length > 0 ? Integer.parseInt(args[0]) : 10;
        for (int i = 0; i < 3; i++) new Thread(new LockTest(0, i)).start();
        new Thread(new LockTest(1, 0)).start();
        long end = System.currentTimeMillis() + secs * 1000L;
        int loops = 0;
        while (System.currentTimeMillis() < end && errors == 0) {
            acquire("/m/" + (loops++ & 127), 3);
            if ((loops & 63) == 0) { try { Thread.sleep(1); } catch (InterruptedException e) { } }
        }
        alive = false;
        try { Thread.sleep(200); } catch (InterruptedException e) { }
        System.out.println("done loops=" + loops + " acquires=" + acquires + " spawned=" + spawned + " finished=" + finished + " errors=" + errors);
    }
}
