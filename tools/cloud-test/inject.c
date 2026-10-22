/* test-only LD_PRELOAD for running the runtime under qemu with SDL dummy video:
 * INJECT_KEYS="ms:sym:d|u,..." scripted key events; SHOT_DIR + SHOT_MS dumps frames */
#define _GNU_SOURCE
#include <SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { Uint32 t; int sym; int down; } Ev;
static Ev evs[512]; static int nev = -1, pos;
static Uint32 t0;
static void load(void) {
  const char *s = getenv("INJECT_KEYS"); nev = 0; t0 = SDL_GetTicks();
  while (s && *s && nev < 512) { unsigned t; int sym; char c;
    if (sscanf(s, "%u:%d:%c", &t, &sym, &c) == 3) { evs[nev].t = t; evs[nev].sym = sym; evs[nev].down = c == 'd'; nev++; }
    s = strchr(s, ','); if (s) s++; }
}
static SDL_Surface *shot_surface; /* last flipped screen not yet saved */
static Uint32 shot_last; static int shot_n, shot_any;
static void shot_maybe(SDL_Surface *s, Uint32 now) {
  const char *dir = getenv("SHOT_DIR");
  Uint32 ms = (Uint32) atoi(getenv("SHOT_MS") ? getenv("SHOT_MS") : "1000");
  if (!dir) return;
  if (s) shot_surface = s;
  if (shot_surface && (!shot_any || now - shot_last >= ms)) {
    char p[512]; snprintf(p, sizeof p, "%s/f%04d_%06u.bmp", dir, shot_n++, now);
    SDL_SaveBMP(shot_surface, p); shot_last = now; shot_any = 1; shot_surface = NULL; }
}
int SDL_PollEvent(SDL_Event *e) {
  static int (*real)(SDL_Event *); Uint32 now;
  if (!real) real = dlsym(RTLD_NEXT, "SDL_PollEvent");
  if (nev < 0) load();
  now = SDL_GetTicks() - t0;
  /* idle screens flip rarely: save the last flipped frame once its turn comes */
  shot_maybe(NULL, SDL_GetTicks());
  if (pos < nev && evs[pos].t <= now) {
    if (e) { memset(e, 0, sizeof *e); e->type = evs[pos].down ? SDL_KEYDOWN : SDL_KEYUP;
      e->key.state = evs[pos].down ? SDL_PRESSED : SDL_RELEASED; e->key.keysym.sym = evs[pos].sym;
      fprintf(stderr, "[inject] t=%u sym=%d %s\n", now, evs[pos].sym, evs[pos].down ? "down" : "up"); }
    pos++; return 1;
  }
  return real(e);
}
int SDL_Flip(SDL_Surface *s) {
  static int (*real)(SDL_Surface *);
  if (!real) real = dlsym(RTLD_NEXT, "SDL_Flip");
  shot_maybe(s, SDL_GetTicks());
  return real(s);
}
/* the launcher blocks in SDL_WaitEvent: poll so scripted keys get through */
int SDL_WaitEvent(SDL_Event *e) {
  for (;;) {
    if (SDL_PollEvent(e)) return 1;
    SDL_Delay(10);
  }
}
