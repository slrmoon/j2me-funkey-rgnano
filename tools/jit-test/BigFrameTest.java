/* Methods with many locals (more than 64 words of locals + stack), monitors
 * inside their loops, and code that runs for the first time long after the
 * method was compiled. The same computation on an array is the reference. */
public class BigFrameTest implements Runnable {
    static volatile boolean alive = true;
    static int errors, shared, rare;
    static final Object lock = new Object();
    StringBuffer sb = new StringBuffer();
    int id;
    BigFrameTest(int i) { id = i; }

    int big(int seed, int n, boolean late) {
        int v0 = seed + 1;
        int v1 = seed + 4;
        int v2 = seed + 7;
        int v3 = seed + 10;
        int v4 = seed + 13;
        int v5 = seed + 16;
        int v6 = seed + 19;
        int v7 = seed + 22;
        int v8 = seed + 25;
        int v9 = seed + 28;
        int v10 = seed + 31;
        int v11 = seed + 34;
        int v12 = seed + 37;
        int v13 = seed + 40;
        int v14 = seed + 43;
        int v15 = seed + 46;
        int v16 = seed + 49;
        int v17 = seed + 52;
        int v18 = seed + 55;
        int v19 = seed + 58;
        int v20 = seed + 61;
        int v21 = seed + 64;
        int v22 = seed + 67;
        int v23 = seed + 70;
        int v24 = seed + 73;
        int v25 = seed + 76;
        int v26 = seed + 79;
        int v27 = seed + 82;
        int v28 = seed + 85;
        int v29 = seed + 88;
        int v30 = seed + 91;
        int v31 = seed + 94;
        int v32 = seed + 97;
        int v33 = seed + 100;
        int v34 = seed + 103;
        int v35 = seed + 106;
        int v36 = seed + 109;
        int v37 = seed + 112;
        int v38 = seed + 115;
        int v39 = seed + 118;
        int v40 = seed + 121;
        int v41 = seed + 124;
        int v42 = seed + 127;
        int v43 = seed + 130;
        int v44 = seed + 133;
        int v45 = seed + 136;
        int v46 = seed + 139;
        int v47 = seed + 142;
        int v48 = seed + 145;
        int v49 = seed + 148;
        int v50 = seed + 151;
        int v51 = seed + 154;
        int v52 = seed + 157;
        int v53 = seed + 160;
        int v54 = seed + 163;
        int v55 = seed + 166;
        int v56 = seed + 169;
        int v57 = seed + 172;
        int v58 = seed + 175;
        int v59 = seed + 178;
        int v60 = seed + 181;
        int v61 = seed + 184;
        int v62 = seed + 187;
        int v63 = seed + 190;
        int v64 = seed + 193;
        int v65 = seed + 196;
        int v66 = seed + 199;
        int v67 = seed + 202;
        int v68 = seed + 205;
        int v69 = seed + 208;
        int v70 = seed + 211;
        int v71 = seed + 214;
        long acc = 0;
        for (int i = 0; i < n; i++) {
            v0 += (v7 ^ i) & 15;
            v1 += (v8 ^ i) & 15;
            v2 += (v9 ^ i) & 15;
            v3 += (v10 ^ i) & 15;
            v4 += (v11 ^ i) & 15;
            v5 += (v12 ^ i) & 15;
            v6 += (v13 ^ i) & 15;
            v7 += (v14 ^ i) & 15;
            v8 += (v15 ^ i) & 15;
            v9 += (v16 ^ i) & 15;
            v10 += (v17 ^ i) & 15;
            v11 += (v18 ^ i) & 15;
            v12 += (v19 ^ i) & 15;
            v13 += (v20 ^ i) & 15;
            v14 += (v21 ^ i) & 15;
            v15 += (v22 ^ i) & 15;
            v16 += (v23 ^ i) & 15;
            v17 += (v24 ^ i) & 15;
            v18 += (v25 ^ i) & 15;
            v19 += (v26 ^ i) & 15;
            v20 += (v27 ^ i) & 15;
            v21 += (v28 ^ i) & 15;
            v22 += (v29 ^ i) & 15;
            v23 += (v30 ^ i) & 15;
            v24 += (v31 ^ i) & 15;
            v25 += (v32 ^ i) & 15;
            v26 += (v33 ^ i) & 15;
            v27 += (v34 ^ i) & 15;
            v28 += (v35 ^ i) & 15;
            v29 += (v36 ^ i) & 15;
            v30 += (v37 ^ i) & 15;
            v31 += (v38 ^ i) & 15;
            v32 += (v39 ^ i) & 15;
            v33 += (v40 ^ i) & 15;
            v34 += (v41 ^ i) & 15;
            v35 += (v42 ^ i) & 15;
            v36 += (v43 ^ i) & 15;
            v37 += (v44 ^ i) & 15;
            v38 += (v45 ^ i) & 15;
            v39 += (v46 ^ i) & 15;
            v40 += (v47 ^ i) & 15;
            v41 += (v48 ^ i) & 15;
            v42 += (v49 ^ i) & 15;
            v43 += (v50 ^ i) & 15;
            v44 += (v51 ^ i) & 15;
            v45 += (v52 ^ i) & 15;
            v46 += (v53 ^ i) & 15;
            v47 += (v54 ^ i) & 15;
            v48 += (v55 ^ i) & 15;
            v49 += (v56 ^ i) & 15;
            v50 += (v57 ^ i) & 15;
            v51 += (v58 ^ i) & 15;
            v52 += (v59 ^ i) & 15;
            v53 += (v60 ^ i) & 15;
            v54 += (v61 ^ i) & 15;
            v55 += (v62 ^ i) & 15;
            v56 += (v63 ^ i) & 15;
            v57 += (v64 ^ i) & 15;
            v58 += (v65 ^ i) & 15;
            v59 += (v66 ^ i) & 15;
            v60 += (v67 ^ i) & 15;
            v61 += (v68 ^ i) & 15;
            v62 += (v69 ^ i) & 15;
            v63 += (v70 ^ i) & 15;
            v64 += (v71 ^ i) & 15;
            v65 += (v0 ^ i) & 15;
            v66 += (v1 ^ i) & 15;
            v67 += (v2 ^ i) & 15;
            v68 += (v3 ^ i) & 15;
            v69 += (v4 ^ i) & 15;
            v70 += (v5 ^ i) & 15;
            v71 += (v6 ^ i) & 15;
            if ((i & 7) == 3) { synchronized (lock) { shared++; acc += v5; } }
            if (late && i == n - 1) { sb.setLength(0); sb.append(v9).append('/').append(acc); rare += sb.length() > 0 ? 1 : 0; }
        }
        return (int) acc + v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9 + v10 + v11 + v12 + v13 + v14 + v15 + v16 + v17 + v18 + v19 + v20 + v21 + v22 + v23 + v24 + v25 + v26 + v27 + v28 + v29 + v30 + v31 + v32 + v33 + v34 + v35 + v36 + v37 + v38 + v39 + v40 + v41 + v42 + v43 + v44 + v45 + v46 + v47 + v48 + v49 + v50 + v51 + v52 + v53 + v54 + v55 + v56 + v57 + v58 + v59 + v60 + v61 + v62 + v63 + v64 + v65 + v66 + v67 + v68 + v69 + v70 + v71;
    }
    static int ref(int seed, int n) {
        int[] v = new int[72];
        for (int k = 0; k < v.length; k++) v[k] = seed + k * 3 + 1;
        long acc = 0;
        for (int i = 0; i < n; i++) {
            for (int k = 0; k < v.length; k++) v[k] += (v[(k + 7) % v.length] ^ i) & 15;
            if ((i & 7) == 3) acc += v[5];
        }
        int s = (int) acc;
        for (int k = 0; k < v.length; k++) s += v[k];
        return s;
    }
    public void run() {
        int k = 0;
        while (alive) {
            int seed = id * 1000 + k, n = 5 + k % 40;
            boolean late = k > 3000 && (k % 50) == 0;
            int got = big(seed, n, late), want = ref(seed, n);
            if (got != want) { errors++; System.out.println("FAIL big " + seed + " " + n + " " + got + " " + want); }
            k++;
            if ((k & 127) == 0) Thread.yield();
        }
    }
    public static void main(String[] args) {
        int secs = args.length > 0 ? Integer.parseInt(args[0]) : 8;
        for (int i = 0; i < 3; i++) new Thread(new BigFrameTest(i)).start();
        long end = System.currentTimeMillis() + secs * 1000L;
        while (System.currentTimeMillis() < end && errors == 0) {
            try { Thread.sleep(50); } catch (InterruptedException e) { }
        }
        alive = false;
        try { Thread.sleep(200); } catch (InterruptedException e) { }
        System.out.println("done shared=" + shared + " rare=" + rare + " errors=" + errors);
    }
}
