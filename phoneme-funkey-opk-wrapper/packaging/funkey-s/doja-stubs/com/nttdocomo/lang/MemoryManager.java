package com.nttdocomo.lang;

/** Java heap figures (one partition). */
public final class MemoryManager {
    public static final int JAVA_HEAP = 0;
    private static MemoryManager instance;

    protected MemoryManager() {
    }

    public static MemoryManager getMemoryManager() {
        if (instance == null) {
            instance = new MemoryManager();
        }
        return instance;
    }

    public long[] totalMemory() {
        return new long[] { Runtime.getRuntime().totalMemory() };
    }

    public long[] freeMemory() {
        return new long[] { Runtime.getRuntime().freeMemory() };
    }

    public long[] maxContiguousMemory() {
        return new long[] { Runtime.getRuntime().freeMemory() };
    }

    public int getNumPartitions() {
        return 1;
    }
}
