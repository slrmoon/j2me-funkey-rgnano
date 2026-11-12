/* Render a media file with the fkaudio engine, off line:
 *   fkrender in.mid out.wav [seconds] [synth: 1 wavetable, 2 fm, 3 hybrid] [volume 0-100] [loops]
 * prints the content type, duration and the events with their times. */
#include "fka.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int fka_output_open(void) { return 0; }
void fka_output_close(void) { }
void fka_lock(void) { }
void fka_unlock(void) { }
void PhoneMEProfileThread(void) { }
const char *PhoneMEConfig(const char *n) { return getenv(n); }
static void w32(FILE *o, unsigned v) { fputc(v, o); fputc(v >> 8, o); fputc(v >> 16, o); fputc(v >> 24, o); }
int main(int c, char **v) {
    FILE *f = fopen(v[1], "rb"), *o;
    long n; unsigned char *d; int id, i, frames, done = 0, tail = 0;
    double secs = c > 3 ? atof(v[3]) : 30;
    int synth = c > 4 ? atoi(v[4]) : 0, vol = c > 5 ? atoi(v[5]) : 100, loops = c > 6 ? atoi(v[6]) : 1;
    int16_t buf[256];
    if (!f) return 1;
    fseek(f, 0, 2); n = ftell(f); fseek(f, 0, 0); d = malloc(n);
    if (fread(d, 1, n, f) != (size_t) n) return 1;
    fclose(f);
    fka_init();
    if (synth) fka_set_synth(synth);
    id = fka_player_new(d, (int) n, NULL);
    if (id <= 0) { printf("unsupported\n"); return 2; }
    printf("type %s kind %d duration %.3f s\n", fka_player_content_type(id), fka_player_kind(id), fka_player_get_duration(id) / 1e6);
    fka_player_set_volume(id, vol, 0);
    fka_player_set_loop_count(id, loops);
    fka_player_prefetch(id); fka_player_start(id);
    o = fopen(v[2], "wb");
    fwrite("RIFF", 1, 4, o); w32(o, 0); fwrite("WAVEfmt ", 1, 8, o); w32(o, 16); w32(o, 0x10001); w32(o, FKA_RATE); w32(o, FKA_RATE * 2); w32(o, 0x100002); fwrite("data", 1, 4, o); w32(o, 0);
    frames = (int) (secs * FKA_RATE);
    for (i = 0; i < frames; i += 256) {
        int p, t, dt; int64_t a; int k, loud = 0;
        fka_render(buf, 256);
        fwrite(buf, 2, 256, o);
        while (fka_poll_event(&p, &t, &a, &dt)) { printf("event %d at render %.3f s media %.3f s data %d\n", t, i / (double) FKA_RATE, a / 1e6, dt); if (t == 1 && !dt) done = 1; }
        for (k = 0; k < 256; k++) if (buf[k] > 8 || buf[k] < -8) loud = 1;
        if (done) { tail = loud ? 0 : tail + 256; if (tail > FKA_RATE / 2) { i += 256; break; } }
    }
    n = ftell(o) - 44; fseek(o, 4, 0); w32(o, (unsigned) n + 36); fseek(o, 40, 0); w32(o, (unsigned) n); fclose(o);
    printf("rendered %.3f s\n", i / (double) FKA_RATE);
    return 0;
}
