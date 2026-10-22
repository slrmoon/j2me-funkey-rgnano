/* host/ARM test (no libm floor: the FunKey SDK musl floor is broken): same protocol as tools/doja/mldref/ORender.java (args: in out [rate] [maxsec] [loop]) */
#include "fka_mld.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void w32(FILE *f, unsigned v) { fputc(v & 255, f); fputc(v >> 8 & 255, f); fputc(v >> 16 & 255, f); fputc(v >> 24 & 255, f); }
static void w16(FILE *f, unsigned v) { fputc(v & 255, f); fputc(v >> 8 & 255, f); }
int main(int argc, char **argv) {
  FILE *f = fopen(argv[1], "rb"); long n; unsigned char *d; FkaMld *p; float buf[4096]; int rate = argc > 3 ? atoi(argv[3]) : 22050;
  double maxs = argc > 4 ? atof(argv[4]) : 30; long total = 0, max; short *pcm; long pc = 0;
  fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET); d = malloc(n); fread(d, 1, n, f); fclose(f);
  p = fka_mld_open(d, (int) n, (float) rate); if (!p) { printf("open failed\n"); return 1; }
  if (argc > 5) fka_mld_set_loop(p, atoi(argv[5]));
  max = (long) (rate * maxs); pcm = malloc(sizeof(short) * 2 * (max + 4096));
  while (total < max) {
    int k, i; memset(buf, 0, sizeof buf);
    k = fka_mld_render(p, buf, 2048); if (k < 0) break;
    for (i = 0; i < k * 2; i++) { float v = buf[i]; if (v < -1) v = -1; if (v > 1) v = 1; { double q = (double) (v * 32767.0f) + 0.5; long long t = (long long) q; if ((double) t > q) t--; pcm[pc++] = (short) t; } }
    total += k; fka_mld_take_shift(p);
  }
  f = fopen(argv[2], "wb"); fwrite("RIFF", 1, 4, f); w32(f, 36 + pc * 2); fwrite("WAVEfmt ", 1, 8, f); w32(f, 16); w16(f, 1); w16(f, 2); w32(f, rate); w32(f, rate * 4); w16(f, 4); w16(f, 16); fwrite("data", 1, 4, f); w32(f, pc * 2); fwrite(pcm, 2, pc, f); fclose(f);
  printf("%s -> %ld frames\n", argv[1], total); fka_mld_close(p); return 0;
}
