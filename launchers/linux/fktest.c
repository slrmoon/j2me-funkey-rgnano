/*
 * fktest.so: LD_PRELOAD helper for automated game runs (test-games.py).
 * Not used by normal play.
 *
 *   FKT_KEYS="ms:sym:d|u,..."   scripted SDL key events (ms of game time)
 *   FKT_SHOT_DIR=dir            dump the screen there as BMP ...
 *   FKT_SHOT_MS=2000            ... every this many ms of game time
 *   FKT_SPEED=4                 run the clock N times faster: clocks,
 *                               gettimeofday, time and sleeps are scaled
 *
 * On SIGTERM the last flipped frame is saved as last.bmp before exit.
 *
 * Build (32-bit, like the runtime):
 *   gcc -m32 -O2 -shared -fPIC $(sdl-config --cflags) fktest.c -o fktest.so -ldl
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <SDL.h>
#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/* ------------------------------------------------------------ fast clock */

static double speed = 0; /* 0: not read yet */
#define NCLK 16
static long long base_real[NCLK]; /* first real reading per clock, ns */
static int base_set[NCLK];

static int (*real_clock_gettime)(clockid_t, struct timespec *);
static int (*real_nanosleep)(const struct timespec *, struct timespec *);
static int (*real_clock_nanosleep)(clockid_t, int, const struct timespec *, struct timespec *);

static void init_clock(void) {
    const char *s;
    if (speed != 0) return;
    real_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");
    real_nanosleep = dlsym(RTLD_NEXT, "nanosleep");
    real_clock_nanosleep = dlsym(RTLD_NEXT, "clock_nanosleep");
    s = getenv("FKT_SPEED");
    speed = s ? atof(s) : 1.0;
    if (speed < 0.1 || speed > 100) speed = 1.0;
}

static long long ts_ns(const struct timespec *t) { return t->tv_sec * 1000000000LL + t->tv_nsec; }
static void ns_ts(long long n, struct timespec *t) { t->tv_sec = n / 1000000000LL; t->tv_nsec = n % 1000000000LL; }

/* real -> game time for one clock */
static long long fake_ns(clockid_t c, long long real) {
    int i = (unsigned) c < NCLK ? (int) c : NCLK - 1;
    if (!base_set[i]) { base_real[i] = real; base_set[i] = 1; }
    return base_real[i] + (long long) ((real - base_real[i]) * speed);
}

int clock_gettime(clockid_t c, struct timespec *t) {
    int r;
    init_clock();
    r = real_clock_gettime(c, t);
    if (r == 0 && speed != 1.0 && c != CLOCK_PROCESS_CPUTIME_ID && c != CLOCK_THREAD_CPUTIME_ID)
        ns_ts(fake_ns(c, ts_ns(t)), t);
    return r;
}

int gettimeofday(struct timeval *tv, void *tz) {
    struct timespec t;
    (void) tz;
    if (clock_gettime(CLOCK_REALTIME, &t) != 0) return -1;
    if (tv) { tv->tv_sec = t.tv_sec; tv->tv_usec = t.tv_nsec / 1000; }
    return 0;
}

time_t time(time_t *out) {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    if (out) *out = t.tv_sec;
    return t.tv_sec;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    struct timespec t;
    init_clock();
    if (speed == 1.0 || !req) return real_nanosleep(req, rem);
    ns_ts((long long) (ts_ns(req) / speed), &t);
    if (rem) rem->tv_sec = rem->tv_nsec = 0;
    return real_nanosleep(&t, NULL);
}

int usleep(useconds_t us) {
    struct timespec t;
    ns_ts(us * 1000LL, &t);
    return nanosleep(&t, NULL);
}

int clock_nanosleep(clockid_t c, int flags, const struct timespec *req, struct timespec *rem) {
    struct timespec t, now;
    init_clock();
    if (speed == 1.0 || !req) return real_clock_nanosleep(c, flags, req, rem);
    if (flags & TIMER_ABSTIME) { /* game-time deadline -> relative real sleep */
        long long d;
        clock_gettime(c, &now);
        d = ts_ns(req) - ts_ns(&now);
        if (d <= 0) return 0;
        ns_ts((long long) (d / speed), &t);
        return real_clock_nanosleep(c, 0, &t, NULL);
    }
    ns_ts((long long) (ts_ns(req) / speed), &t);
    return real_clock_nanosleep(c, 0, &t, NULL);
}

/* ------------------------------------------------------- keys and shots */

typedef struct { Uint32 t; int sym; int down; } Ev;
static Ev evs[1024];
static int nev = -1, pos;
static Uint32 t0;

static void load_keys(void) {
    const char *s = getenv("FKT_KEYS");
    nev = 0;
    t0 = SDL_GetTicks();
    while (s && *s && nev < 1024) {
        unsigned t; int sym; char c;
        if (sscanf(s, "%u:%d:%c", &t, &sym, &c) == 3) {
            evs[nev].t = t; evs[nev].sym = sym; evs[nev].down = c == 'd'; nev++;
        }
        s = strchr(s, ',');
        if (s) s++;
    }
}

static SDL_Surface *pending; /* flipped, not saved yet */
static SDL_Surface *screen;  /* last flipped surface */
static Uint32 shot_last;
static int shot_n, shot_any;

static void shot_maybe(SDL_Surface *s, Uint32 now) {
    const char *dir = getenv("FKT_SHOT_DIR");
    const char *ms_s = getenv("FKT_SHOT_MS");
    Uint32 ms = (Uint32) atoi(ms_s ? ms_s : "2000");
    if (!dir) return;
    if (s) pending = s;
    if (pending && (!shot_any || now - shot_last >= ms)) {
        char p[1024];
        snprintf(p, sizeof p, "%s/shot_%03d_%07u.bmp", dir, shot_n++, now - t0);
        SDL_SaveBMP(pending, p);
        shot_last = now; shot_any = 1; pending = NULL;
    }
}

static void on_term(int sig) {
    const char *dir = getenv("FKT_SHOT_DIR");
    (void) sig;
    if (dir && screen) {
        char p[1024];
        snprintf(p, sizeof p, "%s/last.bmp", dir);
        SDL_SaveBMP(screen, p);
    }
    _exit(0);
}

/* the VM ignores SIGTERM: keep our handler whatever it installs */
int sigaction(int sig, const struct sigaction *act, struct sigaction *old) {
    static int (*real)(int, const struct sigaction *, struct sigaction *);
    if (!real) real = dlsym(RTLD_NEXT, "sigaction");
    if (sig == SIGTERM && act && act->sa_handler != on_term && getenv("FKT_SHOT_DIR"))
        return old ? real(sig, NULL, old) : 0;
    return real(sig, act, old);
}

static void hook_term(void) {
    static int done;
    if (!done) {
        struct sigaction sa;
        memset(&sa, 0, sizeof sa);
        sa.sa_handler = on_term;
        sigaction(SIGTERM, &sa, NULL);
        done = 1;
    }
}

__attribute__((constructor)) static void fkt_init(void) { hook_term(); }

int SDL_PollEvent(SDL_Event *e) {
    static int (*real)(SDL_Event *);
    Uint32 now;
    if (!real) real = dlsym(RTLD_NEXT, "SDL_PollEvent");
    if (nev < 0) load_keys();
    now = SDL_GetTicks() - t0;
    shot_maybe(NULL, SDL_GetTicks()); /* idle screens flip rarely */
    if (pos < nev && evs[pos].t <= now) {
        if (e) {
            memset(e, 0, sizeof *e);
            e->type = evs[pos].down ? SDL_KEYDOWN : SDL_KEYUP;
            e->key.state = evs[pos].down ? SDL_PRESSED : SDL_RELEASED;
            e->key.keysym.sym = evs[pos].sym;
            e->key.keysym.unicode = evs[pos].sym < 128 ? evs[pos].sym : 0;
        }
        pos++;
        return 1;
    }
    return real(e);
}

/* the launcher blocks in SDL_WaitEvent: poll so scripted keys get through */
int SDL_WaitEvent(SDL_Event *e) {
    for (;;) {
        if (SDL_PollEvent(e)) return 1;
        SDL_Delay(10);
    }
}

int SDL_Flip(SDL_Surface *s) {
    static int (*real)(SDL_Surface *);
    if (!real) real = dlsym(RTLD_NEXT, "SDL_Flip");
    if (nev < 0) load_keys();
    hook_term(); /* after the VM set up its own handlers */
    screen = s;
    shot_maybe(s, SDL_GetTicks());
    return real(s);
}

void SDL_UpdateRect(SDL_Surface *s, Sint32 x, Sint32 y, Uint32 w, Uint32 h) {
    static void (*real)(SDL_Surface *, Sint32, Sint32, Uint32, Uint32);
    if (!real) real = dlsym(RTLD_NEXT, "SDL_UpdateRect");
    if (nev < 0) load_keys();
    hook_term();
    screen = s;
    shot_maybe(s, SDL_GetTicks());
    real(s, x, y, w, h);
}
