/* Decodes MLDs through fka_decode and plays them looped for 120 s each (catches
 * hangs in the cuepoint-loop/rebase path); build with fka_decode.c fka_mld.c
 * fka_formats.c fka_smaf.c fka_song.c -lm. */
#include "fka_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int main(int c, char **v) {
  for (int i = 1; i < c; i++) {
    FILE *f = fopen(v[i], "rb"); fseek(f, 0, 2); long n = ftell(f); fseek(f, 0, 0); unsigned char *d = malloc(n); fread(d, 1, n, f); fclose(f);
    FkaContent ct; clock_t t0 = clock();
    if (fka_decode(d, (int) n, "audio/x-mld", &ct) != 0) { printf("%s decode fail\n", v[i]); continue; }
    double dec = (double)(clock() - t0) / CLOCKS_PER_SEC;
    FkaPcm *p = &ct.pcm; long long pos = 0, plays = 0, loops = 0; long long total = 0;
    p->mld_loop = 1;
    while (total < 22050LL * 120) {
      const short *s = fka_pcm_at(p, pos);
      if (p->rebase) { pos -= p->rebase; p->rebase = 0; loops++; }
      if (!s) { plays++; pos = 0; if (plays > 50) break; continue; }
      pos++; total++;
    }
    printf("%s len=%lld dec=%.3fs loops=%lld restarts=%lld cpu=%.2fs\n", v[i] + 15, (long long) p->length, dec, loops, plays, (double)(clock() - t0) / CLOCKS_PER_SEC);
    fflush(stdout);
    fka_content_free(&ct);
  }
  return 0;
}
