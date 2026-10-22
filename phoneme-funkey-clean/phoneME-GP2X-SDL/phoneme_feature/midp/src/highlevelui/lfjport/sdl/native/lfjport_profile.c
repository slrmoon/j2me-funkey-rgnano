/*
 * Sampling CPU profiler (Debug > Profiler, PHONEME_PROFILE=1).
 *
 * Per-thread CPU timers (VM thread, and the audio thread from its first
 * callback) send SIGPROF to that thread every 10 ms of its own CPU time,
 * and the handler records the interrupted pc. Thread CPU timers only fire
 * while the thread runs, so they never interrupt a blocked system call; a
 * process-wide ITIMER_PROF did (EINTR in a sleeping thread stalled the
 * game). Every
 * PHONEME_PROFILE_SECONDS (default 10) the VM thread, at a safe point in
 * the event loop, turns the samples into names and prints to the log:
 *   - CPU load of the period,
 *   - time per category (Java JIT / interpreter / GC / compiler / 3D /
 *     2D / display / audio / libc / ...),
 *   - the top functions and JIT-compiled Java methods.
 * Names come from runMidlet's own symbol table (read from /proc/self/exe,
 * C++ names demangled), dladdr() for shared libraries, and the VM for
 * JIT code. Sampling costs one store per 10 ms; the report a few ms.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <elf.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <stdint.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

/* VM side (OS_linux.cpp): name of the JIT-compiled method containing pc */
extern int jvm_describe_compiled_pc(unsigned long pc, char *buf, int size);
extern char *__cxa_demangle(const char *name, char *buf, size_t *len, int *status);

#define PROF_MAX_SAMPLES 8192
#define PROF_TOP 18

static int prof_enabled;
static int prof_period_ms;
static struct timeval prof_last_report;
static struct rusage prof_last_usage;
static unsigned long prof_pcs[PROF_MAX_SAMPLES];
static volatile int prof_count;
static volatile int prof_dropped;

/* ---- sampling -------------------------------------------------------- */

static void prof_signal(int sig, siginfo_t *info, void *context) {
    ucontext_t *uc = (ucontext_t *) context;
    int i;
    (void) sig;
    (void) info;
    i = __sync_fetch_and_add(&prof_count, 1);
    if (i < PROF_MAX_SAMPLES) {
#if defined(__arm__)
        prof_pcs[i] = (unsigned long) uc->uc_mcontext.arm_pc;
#else
        (void) uc;
        prof_pcs[i] = 0;
#endif
    } else {
        prof_dropped++;
    }
}

/* ---- runMidlet symbols ----------------------------------------------- */

typedef struct {
    unsigned long addr;
    const char *name;
} ProfSym;

static ProfSym *prof_syms;
static int prof_nsyms;
static char *prof_strtab;
static unsigned long prof_text_lo, prof_text_hi;

static int prof_sym_cmp(const void *a, const void *b) {
    unsigned long x = ((const ProfSym *) a)->addr, y = ((const ProfSym *) b)->addr;
    return x < y ? -1 : (x > y ? 1 : 0);
}

static void prof_load_symbols(void) {
    FILE *f = fopen("/proc/self/exe", "rb");
    Elf32_Ehdr eh;
    Elf32_Shdr *sh = NULL;
    Elf32_Sym *syms = NULL;
    int i, nsyms = 0, symtab = -1;
    if (f == NULL) return;
    if (fread(&eh, sizeof eh, 1, f) != 1 || memcmp(eh.e_ident, ELFMAG, SELFMAG) != 0 ||
        eh.e_ident[EI_CLASS] != ELFCLASS32 || eh.e_shentsize != sizeof(Elf32_Shdr)) {
        goto out;
    }
    sh = (Elf32_Shdr *) malloc(eh.e_shnum * sizeof(Elf32_Shdr));
    if (sh == NULL || fseek(f, eh.e_shoff, SEEK_SET) != 0 ||
        fread(sh, sizeof(Elf32_Shdr), eh.e_shnum, f) != eh.e_shnum) {
        goto out;
    }
    for (i = 0; i < eh.e_shnum; i++) {
        if (sh[i].sh_type == SHT_SYMTAB) symtab = i;
        if ((sh[i].sh_flags & SHF_EXECINSTR) && sh[i].sh_size > 0) {
            if (prof_text_lo == 0 || sh[i].sh_addr < prof_text_lo) prof_text_lo = sh[i].sh_addr;
            if (sh[i].sh_addr + sh[i].sh_size > prof_text_hi) prof_text_hi = sh[i].sh_addr + sh[i].sh_size;
        }
    }
    if (symtab < 0 || sh[symtab].sh_link >= eh.e_shnum) goto out;   /* stripped */
    nsyms = sh[symtab].sh_size / sizeof(Elf32_Sym);
    syms = (Elf32_Sym *) malloc(sh[symtab].sh_size);
    prof_strtab = (char *) malloc(sh[sh[symtab].sh_link].sh_size + 1);
    prof_syms = (ProfSym *) malloc(nsyms * sizeof(ProfSym));
    if (syms == NULL || prof_strtab == NULL || prof_syms == NULL) goto out;
    if (fseek(f, sh[symtab].sh_offset, SEEK_SET) != 0 ||
        fread(syms, 1, sh[symtab].sh_size, f) != sh[symtab].sh_size ||
        fseek(f, sh[sh[symtab].sh_link].sh_offset, SEEK_SET) != 0 ||
        fread(prof_strtab, 1, sh[sh[symtab].sh_link].sh_size, f) != sh[sh[symtab].sh_link].sh_size) {
        goto out;
    }
    prof_strtab[sh[sh[symtab].sh_link].sh_size] = '\0';
    for (i = 0; i < nsyms; i++) {
        int type = ELF32_ST_TYPE(syms[i].st_info);
        const char *name = prof_strtab + syms[i].st_name;
        if (syms[i].st_value == 0 || syms[i].st_shndx == SHN_UNDEF ||
            syms[i].st_shndx >= eh.e_shnum ||
            !(sh[syms[i].st_shndx].sh_flags & SHF_EXECINSTR) ||
            (type != STT_FUNC && type != STT_NOTYPE) ||
            name[0] == '\0' || name[0] == '$') {   /* $a/$d mapping symbols */
            continue;
        }
        prof_syms[prof_nsyms].addr = syms[i].st_value & ~1UL;
        prof_syms[prof_nsyms].name = name;
        prof_nsyms++;
    }
    qsort(prof_syms, prof_nsyms, sizeof(ProfSym), prof_sym_cmp);
out:
    free(sh);
    free(syms);
    fclose(f);
}

static const char *prof_exe_symbol(unsigned long pc) {
    int lo = 0, hi = prof_nsyms - 1, best = -1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (prof_syms[mid].addr <= pc) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return best >= 0 ? prof_syms[best].name : NULL;
}

/* "_ZN3Foo3barEv" -> "Foo::bar"; plain C names unchanged */
static void prof_pretty(const char *name, char *out, int size) {
    char *d = NULL;
    int status = -1;
    char *p;
    if (name[0] == '_' && name[1] == 'Z') d = __cxa_demangle(name, NULL, NULL, &status);
    snprintf(out, size, "%s", (d != NULL && status == 0) ? d : name);
    free(d);
    p = strchr(out, '(');            /* drop the parameter list */
    if (p != NULL && p != out) *p = '\0';
    if (strncmp(out, "JVM", 3) == 0 && out[3] >= 'A' && out[3] <= 'Z') {
        memmove(out, out + 3, strlen(out + 3) + 1);   /* CLDC's JVM prefix */
    }
}

/* ---- aggregation ----------------------------------------------------- */

typedef struct {
    char name[96];
    int count;
} ProfEntry;

#define PROF_SLOTS 1024
static ProfEntry prof_table[PROF_SLOTS];
static int prof_used;

static void prof_add(ProfEntry *table, int *used, int slots, const char *name) {
    unsigned h = 5381;
    const char *s;
    int i;
    for (s = name; *s; s++) h = h * 33 + (unsigned char) *s;
    for (i = 0; i < slots; i++) {
        ProfEntry *e = &table[(h + i) % slots];
        if (e->count == 0) {
            snprintf(e->name, sizeof e->name, "%s", name);
            e->count = 1;
            (*used)++;
            return;
        }
        if (strcmp(e->name, name) == 0) {
            e->count++;
            return;
        }
    }
}

static int prof_entry_cmp(const void *a, const void *b) {
    return ((const ProfEntry *) b)->count - ((const ProfEntry *) a)->count;
}

static int prof_starts(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static const char *prof_category(const char *name, int jit, const char *lib) {
    if (jit) return "Java (JIT)";
    if (lib != NULL) {
        if (strstr(lib, "libSDL_gfx")) return "2D graphics";
        if (strstr(lib, "libSDL")) return "SDL";
        if (strstr(lib, "libc")) return "libc";
        return "other libs";
    }
    if (name == NULL) return "unknown";
    if (name[0] == 'L' && name[1] >= '0' && name[1] <= '9') return "Java interpreter"; /* asm labels */
    if (strcmp(name, "_init") == 0) return "PLT (library calls)";
    if (prof_starts(name, "bc_impl") || prof_starts(name, "interpreter") ||
        prof_starts(name, "shared_") || prof_starts(name, "invoke") ||
        prof_starts(name, "fast_") || prof_starts(name, "Interpreter")) return "Java interpreter";
    if (strstr(name, "ObjectHeap") || strstr(name, "Mark") || strstr(name, "Compact") ||
        strstr(name, "GC") || strstr(name, "Scavenge")) return "GC";
    if (strstr(name, "Compiler") || strstr(name, "CodeGenerator") || strstr(name, "Assembler") ||
        strstr(name, "VirtualStackFrame") || strstr(name, "RegisterAllocator") ||
        strstr(name, "Compilation") || strstr(name, "Value::")) return "JIT compiler";
    if (prof_starts(name, "m3g") || prof_starts(name, "ngl") || prof_starts(name, "funkey_m3g") ||
        prof_starts(name, "Java_javax_microedition_m3g")) return "3D (M3G)";
    if (prof_starts(name, "scale_") || prof_starts(name, "present_") ||
        prof_starts(name, "rotate_") || prof_starts(name, "lfjport")) return "display output";
    if (prof_starts(name, "EAS") || prof_starts(name, "eas_") || prof_starts(name, "fka") ||
        prof_starts(name, "WT_") || prof_starts(name, "FM_") || prof_starts(name, "VMs") ||
        prof_starts(name, "VM") || prof_starts(name, "mp3") || prof_starts(name, "Decoder_") ||
        prof_starts(name, "minimp3")) return "audio";
    if (prof_starts(name, "gx") || prof_starts(name, "draw") || prof_starts(name, "fill") ||
        prof_starts(name, "putpixel") || prof_starts(name, "blit") ||
        prof_starts(name, "Java_javax_microedition_lcdui") || strstr(name, "Image") ||
        strstr(name, "Graphics")) return "2D graphics";
    if (prof_starts(name, "inflate") || prof_starts(name, "png") || prof_starts(name, "Jar") ||
        strstr(name, "Inflater")) return "resources (zip/png)";
    return "VM / runtime";
}

static int prof_pc_cmp(const void *a, const void *b) {
    unsigned long x = *(const unsigned long *) a, y = *(const unsigned long *) b;
    return x < y ? -1 : (x > y ? 1 : 0);
}

static void prof_add_n(ProfEntry *table, int *used, int slots, const char *name, int n) {
    int k;
    for (k = 0; k < n; k++) prof_add(table, used, slots, name);
}

static void prof_report(double seconds, double cpu) {
    static ProfEntry cats[32];
    int ncats = 0, i, n = prof_count;
    char name[160];
    if (n > PROF_MAX_SAMPLES) n = PROF_MAX_SAMPLES;
    memset(prof_table, 0, sizeof prof_table);
    memset(cats, 0, sizeof cats);
    prof_used = 0;
    /* Sorted, so every function / address is resolved once, not per sample
     * (demangling and dladdr are far too slow to run 1000 times). */
    qsort(prof_pcs, n, sizeof(prof_pcs[0]), prof_pc_cmp);
    for (i = 0; i < n; ) {
        unsigned long pc = prof_pcs[i];
        const char *sym = NULL, *lib = NULL, *cat;
        int jit = 0, run = 1;
        Dl_info info;
        if (pc >= prof_text_lo && pc < prof_text_hi) {
            /* group every sample that falls into the same function */
            sym = prof_exe_symbol(pc);
            while (i + run < n && prof_pcs[i + run] < prof_text_hi &&
                   prof_exe_symbol(prof_pcs[i + run]) == sym) run++;
            if (sym != NULL) prof_pretty(sym, name, sizeof name);
            else snprintf(name, sizeof name, "runMidlet+0x%lx", pc);
        } else {
            while (i + run < n && prof_pcs[i + run] == pc) run++;
            if (dladdr((void *) pc, &info) != 0 && info.dli_fname != NULL) {
                const char *base = strrchr(info.dli_fname, '/');
                lib = base != NULL ? base + 1 : info.dli_fname;
                if (info.dli_sname != NULL) {
                    char pretty[128];
                    prof_pretty(info.dli_sname, pretty, sizeof pretty);
                    snprintf(name, sizeof name, "%s (%s)", pretty, lib);
                } else {
                    snprintf(name, sizeof name, "%s", lib);
                }
            } else if (jvm_describe_compiled_pc(pc, name, sizeof name)) {
                jit = 1;
            } else {
                snprintf(name, sizeof name, "?");
            }
        }
        cat = prof_category(sym != NULL ? name : NULL, jit, lib);
        prof_add_n(prof_table, &prof_used, PROF_SLOTS, name, run);
        prof_add_n(cats, &ncats, 32, cat, run);
        i += run;
    }
    qsort(prof_table, PROF_SLOTS, sizeof(ProfEntry), prof_entry_cmp);
    qsort(cats, 32, sizeof(ProfEntry), prof_entry_cmp);
    fprintf(stderr, "[profile] %.1f s, CPU %.0f%%, %d samples%s\n", seconds, cpu, n,
            prof_dropped ? " (some dropped)" : "");
    if (n == 0) return;
    fprintf(stderr, "[profile] by area:");
    for (i = 0; i < 32 && cats[i].count > 0; i++) {
        fprintf(stderr, "%s %s %.0f%%", i ? "," : "", cats[i].name, 100.0 * cats[i].count / n);
    }
    fprintf(stderr, "\n");
    for (i = 0; i < PROF_TOP && prof_table[i].count > 0; i++) {
        fprintf(stderr, "[profile] %5.1f%%  %s\n", 100.0 * prof_table[i].count / n,
                prof_table[i].name);
    }
}

/* ---- public ---------------------------------------------------------- */

/* SIGPROF to the calling thread every 10 ms of its CPU time. Raw syscalls:
 * the SDK's musl predates SIGEV_THREAD_ID; this is the kernel's layout. */
struct prof_kernel_sigevent {
    int value;
    int signo;
    int notify;
    int tid;
    int pad[12];
};
#define PROF_SIGEV_THREAD_ID 4

static int prof_start_thread_timer(void) {
    struct prof_kernel_sigevent sev;
    struct itimerspec its;
    int timer = -1;
    memset(&sev, 0, sizeof sev);
    sev.signo = SIGPROF;
    sev.notify = PROF_SIGEV_THREAD_ID;
    sev.tid = (int) syscall(SYS_gettid);
    if (syscall(SYS_timer_create, CLOCK_THREAD_CPUTIME_ID, &sev, &timer) != 0) return 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 10 * 1000 * 1000;
    its.it_value = its.it_interval;
    /* libc's timer_settime picks the time32/time64 syscall the kernel has
     * (the FunKey kernel is 4.14); a signal timer's timer_t is its id */
    return timer_settime((timer_t) (intptr_t) timer, 0, &its, NULL) == 0;
}

void PhoneMEProfileInit(void) {
    const char *on = getenv("PHONEME_PROFILE");
    const char *secs = getenv("PHONEME_PROFILE_SECONDS");
    struct sigaction sa;
    if (on == NULL || on[0] == '\0' || on[0] == '0') return;
    prof_period_ms = (secs != NULL && atoi(secs) > 0) ? atoi(secs) * 1000 : 10000;
    prof_load_symbols();
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = prof_signal;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPROF, &sa, NULL) != 0) return;
    if (!prof_start_thread_timer()) {
        fprintf(stderr, "[profile] cannot create a thread CPU timer\n");
        return;
    }
    gettimeofday(&prof_last_report, NULL);
    getrusage(RUSAGE_SELF, &prof_last_usage);
    prof_enabled = 1;
    fprintf(stderr, "[profile] on: report every %d s, %d symbols\n",
            prof_period_ms / 1000, prof_nsyms);
}

/* Called once from another thread worth sampling (the audio callback) */
void PhoneMEProfileThread(void) {
    static volatile int audio_started;
    if (!prof_enabled || audio_started) return;
    audio_started = 1;
    prof_start_thread_timer();
}

/* VM thread, from the event loop (a safe point: no GC or compilation runs) */
void PhoneMEProfileTick(void) {
    struct timeval now;
    struct rusage usage;
    long ms;
    double cpu_ms;
    if (!prof_enabled) return;
    gettimeofday(&now, NULL);
    ms = (now.tv_sec - prof_last_report.tv_sec) * 1000L +
         (now.tv_usec - prof_last_report.tv_usec) / 1000L;
    if (ms < prof_period_ms) return;
    getrusage(RUSAGE_SELF, &usage);
    cpu_ms = (usage.ru_utime.tv_sec - prof_last_usage.ru_utime.tv_sec) * 1000.0 +
             (usage.ru_utime.tv_usec - prof_last_usage.ru_utime.tv_usec) / 1000.0 +
             (usage.ru_stime.tv_sec - prof_last_usage.ru_stime.tv_sec) * 1000.0 +
             (usage.ru_stime.tv_usec - prof_last_usage.ru_stime.tv_usec) / 1000.0;
    prof_report(ms / 1000.0, ms > 0 ? 100.0 * cpu_ms / ms : 0.0);
    fflush(stderr);
    prof_count = 0;
    prof_dropped = 0;
    prof_last_report = now;
    prof_last_usage = usage;
}
