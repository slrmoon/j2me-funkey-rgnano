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
int SDL_PollEvent(SDL_Event *e) {
  static int (*real)(SDL_Event *); Uint32 now;
  if (!real) real = dlsym(RTLD_NEXT, "SDL_PollEvent");
  if (nev < 0) load();
  now = SDL_GetTicks() - t0;
  if (pos < nev && evs[pos].t <= now) {
    if (e) { memset(e, 0, sizeof *e); e->type = evs[pos].down ? SDL_KEYDOWN : SDL_KEYUP;
      e->key.state = evs[pos].down ? SDL_PRESSED : SDL_RELEASED; e->key.keysym.sym = evs[pos].sym;
      fprintf(stderr, "[inject] t=%u sym=%d %s\n", now, evs[pos].sym, evs[pos].down ? "down" : "up"); }
    pos++; return 1;
  }
  return real(e);
}
int SDL_Flip(SDL_Surface *s) {
  static int (*real)(SDL_Surface *); static Uint32 last; static int n;
  const char *dir = getenv("SHOT_DIR"); Uint32 now = SDL_GetTicks();
  if (!real) real = dlsym(RTLD_NEXT, "SDL_Flip");
  if (dir && now - last >= (Uint32) atoi(getenv("SHOT_MS") ? getenv("SHOT_MS") : "1000")) {
    char p[512]; snprintf(p, sizeof p, "%s/f%04d_%06u.bmp", dir, n++, now); SDL_SaveBMP(s, p); last = now; }
  return real(s);
}
