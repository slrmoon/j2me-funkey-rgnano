/*
 * phoneME: Linux launcher of the desktop runtime (run.sh -> bin/runMidlet).
 *
 *   ./phoneME                 game list and settings (display, sound,
 *                                controls, logs), as the Windows launcher
 *   ./phoneME game.jar        start the game right away
 *
 * Settings live in $XDG_DATA_HOME/phoneme-j2me/settings.ini, a game can
 * have its own copy in .../games/<game>.ini. The debug switches go to
 * .../debug.cfg, which run.sh and the runtime menu read as well, so the
 * launcher and the menu (Esc in a game) always show the same choices.
 *
 * The window is drawn with SDL 1.2 and SDL_gfx, the two libraries the
 * runtime itself needs, so the package gains no dependency. Text uses the
 * SDL_gfx 8x8 font plus the launcher font of the FunKey (Cyrillic, kana,
 * CJK) so game names show as they are.
 *
 * Build: see package.sh
 *        (cc launcher.c -o phoneME $(sdl-config --cflags --libs) -lSDL_gfx -lz)
 */
#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <SDL_gfxPrimitives_font.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <zlib.h>

#include "../funkey/pm-unicode-font.h"

#define BUF 1024
#define MAX_GAMES 2048
#define MAX_BIND 3

static char g_home[BUF];        /* the package folder (run.sh lives here) */
static char g_data[BUF];        /* ~/.local/share/phoneme-j2me */
static char g_ini[BUF];         /* g_data/settings.ini */

/* ===================================================================== */
/*  small helpers                                                        */
/* ===================================================================== */

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (p == NULL) { fprintf(stderr, "out of memory\n"); exit(1); }
    return p;
}

static void copy_str(char *dst, size_t n, const char *v) {
    strncpy(dst, v, n - 1);
    dst[n - 1] = 0;
}

static void mkdirs(const char *path) {
    char tmp[BUF];
    size_t i;
    copy_str(tmp, sizeof(tmp), path);
    for (i = 1; tmp[i]; i++) {
        if (tmp[i] == '/') { tmp[i] = 0; mkdir(tmp, 0777); tmp[i] = '/'; }
    }
    mkdir(tmp, 0777);
}

static int is_dir(const char *p) {
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static int is_file(const char *p) {
    struct stat st;
    return stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

static int read_file(const char *path, unsigned char **out, size_t *len) {
    FILE *f = fopen(path, "rb");
    long n;
    unsigned char *d;
    if (f == NULL) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    n = ftell(f);
    if (n < 0 || n > 64L * 1024 * 1024) { fclose(f); return 0; }
    rewind(f);
    d = xmalloc((size_t)n + 1);
    if (fread(d, 1, (size_t)n, f) != (size_t)n) { free(d); fclose(f); return 0; }
    fclose(f);
    d[n] = 0;
    *out = d;
    if (len) *len = (size_t)n;
    return 1;
}

static int write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "wb");
    if (f == NULL) return 0;
    fputs(text, f);
    fclose(f);
    return 1;
}

/* "~/games" -> "/home/me/games" */
static void expand_path(const char *in, char *out, size_t n) {
    const char *home = getenv("HOME");
    if (in[0] == '~' && (in[1] == '/' || in[1] == 0) && home != NULL)
        snprintf(out, n, "%s%s", home, in + 1);
    else
        copy_str(out, n, in);
}

static const char *base_name(const char *p) {
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

/* the file name without its extension */
static void stem_of(const char *path, char *out, size_t n) {
    const char *b = base_name(path), *dot = strrchr(b, '.');
    size_t len = dot ? (size_t)(dot - b) : strlen(b);
    if (len >= n) len = n - 1;
    memcpy(out, b, len);
    out[len] = 0;
}

/* ===================================================================== */
/*  text (SDL_gfx 8x8 font + the FunKey launcher font), drawn scaled     */
/* ===================================================================== */

static SDL_Surface *scr;
static int UI = 2;              /* pixel scale of the whole window */
#define GW (8 * UI)             /* glyph box */
#define GH (8 * UI)
#define LINE (11 * UI)          /* line height */

static const unsigned char *font_bitmap(uint32_t cp) {
    int i, count = (int)(uintptr_t)PmFontBitmaps[0];
    unsigned char hi = (unsigned char)((cp >> 8) & 0xff), lo = (unsigned char)(cp & 0xff);
    for (i = 1; i <= count; i++) {
        const unsigned char *f = PmFontBitmaps[i];
        if (hi == f[PM_FONT_CODE_RANGE_HIGH] &&
            lo >= f[PM_FONT_CODE_FIRST_LOW] && lo <= f[PM_FONT_CODE_LAST_LOW]) return f;
    }
    return NULL;
}

static uint32_t next_utf8(const char **text) {
    const unsigned char *s = (const unsigned char *)*text;
    uint32_t cp;
    if (s[0] < 0x80) { *text += 1; return s[0]; }
    if ((s[0] & 0xe0) == 0xc0 && (s[1] & 0xc0) == 0x80) {
        cp = ((uint32_t)(s[0] & 0x1f) << 6) | (uint32_t)(s[1] & 0x3f);
        *text += 2;
        return cp;
    }
    if ((s[0] & 0xf0) == 0xe0 && (s[1] & 0xc0) == 0x80 && (s[2] & 0xc0) == 0x80) {
        cp = ((uint32_t)(s[0] & 0x0f) << 12) | ((uint32_t)(s[1] & 0x3f) << 6) |
             (uint32_t)(s[2] & 0x3f);
        *text += 3;
        return cp;
    }
    if ((s[0] & 0xf8) == 0xf0 && (s[1] & 0xc0) == 0x80 && (s[2] & 0xc0) == 0x80 &&
        (s[3] & 0xc0) == 0x80) {
        *text += 4;
        return 0xfffd;
    }
    *text += 1;
    return '?';
}

static void fill(int x, int y, int w, int h, Uint32 c) {
    SDL_Rect r;
    if (w <= 0 || h <= 0) return;
    r.x = (Sint16)x; r.y = (Sint16)y;
    r.w = (Uint16)w; r.h = (Uint16)h;
    SDL_FillRect(scr, &r, c);
}

/* one glyph, every font pixel a UI x UI block */
static void glyph(int x, int y, uint32_t cp, Uint32 c) {
    int row, col, w = 8, h = 8;
    const unsigned char *f;
    if (cp < 0x80) {
        const unsigned char *g = gfxPrimitivesFontdata + cp * 8;
        for (row = 0; row < 8; row++)
            for (col = 0; col < 8; col++)
                if (g[row] & (0x80 >> col)) fill(x + col * UI, y + row * UI, UI, UI, c);
        return;
    }
    f = font_bitmap(cp);
    if (f == NULL) { glyph(x, y, '?', c); return; }
    w = f[PM_FONT_WIDTH];
    h = f[PM_FONT_HEIGHT];
    {
        unsigned long first = (unsigned long)((int)cp -
            ((f[PM_FONT_CODE_RANGE_HIGH] << 8) | f[PM_FONT_CODE_FIRST_LOW])) *
            (unsigned long)w * (unsigned long)h;
        unsigned long map = (((unsigned long)(f[PM_FONT_CODE_LAST_LOW] -
            f[PM_FONT_CODE_FIRST_LOW] + 1) * (unsigned long)w * (unsigned long)h) + 7) >> 3;
        for (row = 0; row < h; row++) {
            for (col = 0; col < w; col++) {
                unsigned long pix = first + (unsigned long)row * w + col;
                if ((pix >> 3) < map && (f[PM_FONT_DATA + (pix >> 3)] & (0x80 >> (pix & 7))))
                    fill(x + col * UI, y + row * UI, UI, UI, c);
            }
        }
    }
}

static int text_w(const char *s) {
    int w = 0;
    while (s && *s) { next_utf8(&s); w += GW; }
    return w;
}

static void text(int x, int y, const char *s, Uint32 c) {
    while (s && *s) {
        uint32_t cp = next_utf8(&s);
        if (cp != ' ') glyph(x, y, cp, c);
        x += GW;
    }
}

/* text cut to a pixel width, with an ellipsis when it does not fit */
static void text_fit(int x, int y, const char *s, int w, Uint32 c) {
    int n = w / GW, i = 0;
    const char *p = s;
    if (n <= 0) return;
    while (*p && i < n) { next_utf8(&p); i++; }
    if (*p == 0) { text(x, y, s, c); return; }
    p = s;
    i = 0;
    while (*p && i < n - 1) {
        uint32_t cp = next_utf8(&p);
        if (cp != ' ') glyph(x + i * GW, y, cp, c);
        i++;
    }
    glyph(x + i * GW, y, '~', c);
}

/* ===================================================================== */
/*  jar / jam                                                            */
/* ===================================================================== */

typedef struct { unsigned char *data; size_t size, cd; unsigned entries; } Zip;

static unsigned u16(const unsigned char *p) { return p[0] | (p[1] << 8); }
static unsigned long u32(const unsigned char *p) {
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

static int zip_open(Zip *z, const char *path) {
    long i;
    memset(z, 0, sizeof(*z));
    if (!read_file(path, &z->data, &z->size)) return 0;
    for (i = (long)z->size - 22; i >= 0 && i >= (long)z->size - 22 - 65535; i--) {
        if (u32(z->data + i) == 0x06054b50) {
            z->entries = u16(z->data + i + 10);
            z->cd = (size_t)u32(z->data + i + 16);
            if (z->cd < z->size) return 1;
            break;
        }
    }
    free(z->data);
    z->data = NULL;
    return 0;
}

static void zip_close(Zip *z) { free(z->data); z->data = NULL; }

static unsigned char *zip_read(Zip *z, const char *name, int nocase) {
    size_t p = z->cd, nlen = strlen(name);
    unsigned i;
    for (i = 0; i < z->entries && p + 46 <= z->size; i++) {
        const unsigned char *c = z->data + p;
        unsigned method, fn, ex, cm;
        size_t csize, usize, lho;
        if (u32(c) != 0x02014b50) break;
        method = u16(c + 10);
        csize = (size_t)u32(c + 20);
        usize = (size_t)u32(c + 24);
        fn = u16(c + 28); ex = u16(c + 30); cm = u16(c + 32);
        lho = (size_t)u32(c + 42);
        if (fn == nlen && p + 46 + fn <= z->size &&
            (nocase ? strncasecmp((const char *)c + 46, name, nlen) == 0
                    : memcmp(c + 46, name, nlen) == 0)) {
            const unsigned char *l = z->data + lho;
            size_t start;
            unsigned char *out;
            if (lho + 30 > z->size || u32(l) != 0x04034b50) return NULL;
            start = lho + 30 + u16(l + 26) + u16(l + 28);
            if (start + csize > z->size || usize > 16u * 1024 * 1024) return NULL;
            out = xmalloc(usize + 1);
            if (method == 0) {
                memcpy(out, z->data + start, usize);
            } else if (method == 8) {
                z_stream s;
                memset(&s, 0, sizeof(s));
                if (inflateInit2(&s, -MAX_WBITS) != Z_OK) { free(out); return NULL; }
                s.next_in = z->data + start;
                s.avail_in = (uInt)csize;
                s.next_out = out;
                s.avail_out = (uInt)usize;
                inflate(&s, Z_FINISH);
                usize = s.total_out;
                inflateEnd(&s);
            } else {
                free(out);
                return NULL;
            }
            out[usize] = 0;
            return out;
        }
        p += 46 + fn + ex + cm;
    }
    return NULL;
}

/* value of a "Key: value" line (manifest, jad) or "Key = value" (jam) */
static int desc_value(const char *text, const char *key, char *out, size_t n) {
    size_t klen = strlen(key);
    const char *p = text;
    out[0] = 0;
    while (p && *p) {
        const char *e = strchr(p, '\n');
        size_t len = e ? (size_t)(e - p) : strlen(p);
        if (strncasecmp(p, key, klen) == 0) {
            const char *v = p + klen;
            while (*v == ' ') v++;
            if (*v == ':' || *v == '=') {
                size_t vl;
                v++;
                while (*v == ' ') v++;
                vl = (size_t)(p + len - v);
                while (vl > 0 && (v[vl - 1] == '\r' || v[vl - 1] == ' ')) vl--;
                if (vl >= n) vl = n - 1;
                memcpy(out, v, vl);
                out[vl] = 0;
                return out[0] != 0;
            }
        }
        if (!e) break;
        p = e + 1;
    }
    return 0;
}

/* ===================================================================== */
/*  settings (same keys as the Windows launcher's settings.ini)          */
/* ===================================================================== */

#define ACT_SOFT1 -1001
#define ACT_SOFT2 -1002
#define ACT_MENU  1000
#define ACT_FULL  1001

typedef struct {
    const char *id;
    const char *label;
    int code;
    int defaults[MAX_BIND];
} Action;

static const Action Actions[] = {
    { "up",    "Up",              -1, { SDLK_UP, 'w', 0 } },
    { "down",  "Down",            -2, { SDLK_DOWN, 's', 0 } },
    { "left",  "Left",            -3, { SDLK_LEFT, 'a', 0 } },
    { "right", "Right",           -4, { SDLK_RIGHT, 'd', 0 } },
    { "fire",  "Fire / OK",       -5, { SDLK_RETURN, SDLK_SPACE, 0 } },
    { "soft1", "Left soft key",   ACT_SOFT1, { SDLK_F1, SDLK_TAB, 'q' } },
    { "soft2", "Right soft key",  ACT_SOFT2, { SDLK_F2, SDLK_BACKSPACE, 'e' } },
    { "1",     "1",               '1', { '1', SDLK_KP7, 0 } },
    { "2",     "2",               '2', { '2', SDLK_KP8, 0 } },
    { "3",     "3",               '3', { '3', SDLK_KP9, 0 } },
    { "4",     "4",               '4', { '4', SDLK_KP4, 0 } },
    { "5",     "5",               '5', { '5', SDLK_KP5, 0 } },
    { "6",     "6",               '6', { '6', SDLK_KP6, 0 } },
    { "7",     "7",               '7', { '7', SDLK_KP1, 0 } },
    { "8",     "8",               '8', { '8', SDLK_KP2, 0 } },
    { "9",     "9",               '9', { '9', SDLK_KP3, 0 } },
    { "0",     "0",               '0', { '0', SDLK_KP0, 0 } },
    { "star",  "*",               '*', { SDLK_KP_MULTIPLY, 'z', 0 } },
    { "pound", "#",               '#', { SDLK_KP_PLUS, 'x', 0 } },
    { "clear", "Clear (C)",       -8,  { SDLK_DELETE, 0, 0 } },
    { "send",  "Call (green)",    -10, { SDLK_HOME, 0, 0 } },
    { "end",   "Hang up (red)",   -11, { SDLK_END, 0, 0 } },
    { "gamea", "Game A",          -13, { SDLK_F9, 0, 0 } },
    { "gameb", "Game B",          -14, { SDLK_F10, 0, 0 } },
    { "gamec", "Game C",          -15, { SDLK_PAGEUP, 0, 0 } },
    { "gamed", "Game D",          -16, { SDLK_PAGEDOWN, SDLK_F12, 0 } },
    { "menu",  "[Runtime menu]",  ACT_MENU, { SDLK_ESCAPE, 0, 0 } },
    { "full",  "[Full screen]",   ACT_FULL, { SDLK_F11, 0, 0 } }
};
#define ACTION_COUNT ((int)(sizeof(Actions) / sizeof(Actions[0])))

static const struct { const char *id, *label; int soft1, soft2; } Phones[] = {
    { "standard", "Nokia / SE / Samsung (-6/-7)", -6, -7 },
    { "motorola", "Motorola (-21/-22)", -21, -22 },
    { "moto-old", "Motorola old (21/22)", 21, 22 },
    { "siemens",  "Siemens (-1/-4)", -1, -4 }
};
#define PHONE_COUNT ((int)(sizeof(Phones) / sizeof(Phones[0])))

typedef struct {
    int scale, fullscreen;
    char mode[16], filter[16], lcd[32], fps[8];
    int heap, jit, verify;
    char vmargs[256];
    int mute, volume;
    char synth[16];
    int log, log_keys, log_events, log_display, show_fps, log_audio;
    int zoom;
    char pan[24];
    char phone[16];
    int keys[ACTION_COUNT][MAX_BIND];
    char games_dir[BUF];
} Settings;

static void settings_default(Settings *s) {
    int i, j;
    memset(s, 0, sizeof(*s));
    s->scale = 3;
    strcpy(s->mode, "fit");
    strcpy(s->filter, "sharp");
    strcpy(s->fps, "30");
    s->jit = 1;
    s->volume = 100;
    strcpy(s->synth, "hybrid");
    s->log = 1;
    strcpy(s->phone, "standard");
    for (i = 0; i < ACTION_COUNT; i++)
        for (j = 0; j < MAX_BIND; j++) s->keys[i][j] = Actions[i].defaults[j];
    snprintf(s->games_dir, BUF, "%s/games", g_home);
}

static void keys_parse(Settings *s, const char *v) {
    while (*v) {
        char id[32];
        size_t n = 0;
        int i, a = -1, k = 0;
        while (*v == ' ' || *v == ';') v++;
        while (*v && *v != '=' && *v != ';' && n < sizeof(id) - 1) id[n++] = *v++;
        id[n] = 0;
        if (*v != '=') { while (*v && *v != ';') v++; continue; }
        v++;
        for (i = 0; i < ACTION_COUNT; i++) if (strcmp(Actions[i].id, id) == 0) a = i;
        if (a >= 0) for (i = 0; i < MAX_BIND; i++) s->keys[a][i] = 0;
        while (*v && *v != ';') {
            int sym = atoi(v);
            if (a >= 0 && k < MAX_BIND && sym > 0) s->keys[a][k++] = sym;
            while (*v && *v != ';' && *v != ',') v++;
            if (*v == ',') v++;
        }
    }
}

static void settings_apply(Settings *s, const char *key, const char *v) {
    if (!strcasecmp(key, "SCALE")) { s->scale = atoi(v); if (s->scale < 1) s->scale = 1;
                                     if (s->scale > 8) s->scale = 8; }
    else if (!strcasecmp(key, "FULLSCREEN")) s->fullscreen = atoi(v) != 0;
    else if (!strcasecmp(key, "MODE")) copy_str(s->mode, sizeof(s->mode), v);
    else if (!strcasecmp(key, "FILTER")) copy_str(s->filter, sizeof(s->filter), v);
    else if (!strcasecmp(key, "LCD")) copy_str(s->lcd, sizeof(s->lcd), v);
    else if (!strcasecmp(key, "FPS")) copy_str(s->fps, sizeof(s->fps), v);
    else if (!strcasecmp(key, "HEAP_MB")) s->heap = atoi(v);
    else if (!strcasecmp(key, "JIT")) s->jit = atoi(v) != 0;
    else if (!strcasecmp(key, "VERIFY")) s->verify = atoi(v) != 0;
    else if (!strcasecmp(key, "VMARGS")) copy_str(s->vmargs, sizeof(s->vmargs), v);
    else if (!strcasecmp(key, "MUTE")) s->mute = atoi(v) != 0;
    else if (!strcasecmp(key, "VOLUME")) s->volume = atoi(v);
    else if (!strcasecmp(key, "SYNTH")) copy_str(s->synth, sizeof(s->synth), v);
    else if (!strcasecmp(key, "LOG")) s->log = atoi(v) != 0;
    else if (!strcasecmp(key, "LOG_KEYS")) s->log_keys = atoi(v) != 0;
    else if (!strcasecmp(key, "LOG_EVENTS")) s->log_events = atoi(v) != 0;
    else if (!strcasecmp(key, "LOG_DISPLAY")) s->log_display = atoi(v) != 0;
    else if (!strcasecmp(key, "SHOW_FPS")) s->show_fps = atoi(v) != 0;
    else if (!strcasecmp(key, "LOG_AUDIO")) s->log_audio = atoi(v) != 0;
    else if (!strcasecmp(key, "ZOOM")) s->zoom = atoi(v);
    else if (!strcasecmp(key, "PAN")) copy_str(s->pan, sizeof(s->pan), v);
    else if (!strcasecmp(key, "PHONE")) copy_str(s->phone, sizeof(s->phone), v);
    else if (!strcasecmp(key, "KEYS")) keys_parse(s, v);
    else if (!strcasecmp(key, "GAMES_DIR")) expand_path(v, s->games_dir, BUF);
}

static int settings_load_file(Settings *s, const char *path) {
    unsigned char *d;
    char *p;
    if (!read_file(path, &d, NULL)) return 0;
    p = (char *)d;
    while (*p) {
        char *e = strchr(p, '\n'), *eq, *k, *t;
        if (e) *e = 0;
        k = p;
        while (*k == ' ' || *k == '\t') k++;
        eq = strchr(k, '=');
        if (*k != '#' && *k != ';' && *k != '[' && eq != NULL) {
            char *v;
            *eq = 0;
            for (t = eq; t > k && (t[-1] == ' ' || t[-1] == '\t'); ) *--t = 0;
            v = eq + 1;
            while (*v == ' ' || *v == '\t') v++;
            t = v + strlen(v);
            while (t > v && (t[-1] == '\r' || t[-1] == ' ' || t[-1] == '\t')) *--t = 0;
            settings_apply(s, k, v);
        }
        if (!e) break;
        p = e + 1;
    }
    free(d);
    return 1;
}

static void keys_format(const Settings *s, char *out, size_t size) {
    size_t o = 0;
    int i, j;
    out[0] = 0;
    for (i = 0; i < ACTION_COUNT && o + 64 < size; i++) {
        o += (size_t)snprintf(out + o, size - o, "%s%s=", i ? ";" : "", Actions[i].id);
        for (j = 0; j < MAX_BIND; j++) {
            if (s->keys[i][j])
                o += (size_t)snprintf(out + o, size - o, "%s%d",
                                      out[o - 1] == '=' ? "" : ",", s->keys[i][j]);
        }
    }
}

static int settings_save_file(const Settings *s, const char *path, int global) {
    char buf[8192], keys[4096];
    int n;
    keys_format(s, keys, sizeof(keys));
    n = snprintf(buf, sizeof(buf),
        "# phoneME J2ME settings (written by the launcher)\n"
        "SCALE = %d\nFULLSCREEN = %d\nMODE = %s\nFILTER = %s\nLCD = %s\n"
        "FPS = %s\nHEAP_MB = %d\nJIT = %d\nVERIFY = %d\nVMARGS = %s\n"
        "MUTE = %d\nVOLUME = %d\nSYNTH = %s\n"
        "LOG = %d\nLOG_KEYS = %d\nLOG_EVENTS = %d\nLOG_DISPLAY = %d\n"
        "SHOW_FPS = %d\nLOG_AUDIO = %d\nZOOM = %d\nPAN = %s\n"
        "PHONE = %s\nKEYS = %s\n",
        s->scale, s->fullscreen, s->mode, s->filter, s->lcd, s->fps, s->heap, s->jit,
        s->verify, s->vmargs, s->mute, s->volume, s->synth, s->log,
        s->log_keys, s->log_events, s->log_display, s->show_fps, s->log_audio, s->zoom,
        s->pan, s->phone, keys);
    if (global && n > 0 && n < (int)sizeof(buf))
        n += snprintf(buf + n, sizeof(buf) - (size_t)n, "GAMES_DIR = %s\n", s->games_dir);
    if (n <= 0 || n >= (int)sizeof(buf)) return 0;
    return write_file(path, buf);
}

/* the VM cannot open a path with non-ASCII characters: run.sh gives such a
 * game an ASCII name, and its files are then out of the launcher's reach */
static int ascii_only(const char *s) {
    const unsigned char *p = (const unsigned char *)s;
    while (*p) { if (*p < 32 || *p > 126) return 0; p++; }
    return 1;
}

/* data/games/<game>.ini, "" when the name is not ASCII */
static void game_ini_path(const char *jar, char *out, size_t n) {
    char stem[BUF];
    stem_of(jar, stem, sizeof(stem));
    if (!ascii_only(stem)) { out[0] = 0; return; }
    snprintf(out, n, "%s/games/%s.ini", g_data, stem);
}

static void settings_for(Settings *s, const char *jar) {
    settings_default(s);
    settings_load_file(s, g_ini);
    if (jar != NULL && jar[0]) {
        char gi[BUF];
        game_ini_path(jar, gi, sizeof(gi));
        if (gi[0]) settings_load_file(s, gi);
    }
}

static int phone_index(const char *id) {
    int i;
    for (i = 0; i < PHONE_COUNT; i++) if (strcmp(Phones[i].id, id) == 0) return i;
    return 0;
}

/* "sym=code;..." for PHONEME_PC_KEYS */
static void pc_keys_env(const Settings *s, char *out, size_t size) {
    size_t o = 0;
    int i, j, p = phone_index(s->phone);
    out[0] = 0;
    for (i = 0; i < ACTION_COUNT; i++) {
        int code = Actions[i].code;
        if (code == ACT_SOFT1) code = Phones[p].soft1;
        if (code == ACT_SOFT2) code = Phones[p].soft2;
        for (j = 0; j < MAX_BIND; j++) {
            if (s->keys[i][j] && o + 24 < size)
                o += (size_t)snprintf(out + o, size - o, "%s%d=%d",
                                      o ? ";" : "", s->keys[i][j], code);
        }
    }
}

/* ===================================================================== */
/*  the runtime menu's files (shared with run.sh and the FunKey)         */
/* ===================================================================== */

static void debug_cfg_write(const Settings *s) {
    char path[BUF], buf[256];
    snprintf(path, sizeof(path), "%s/debug.cfg", g_data);
    snprintf(buf, sizeof(buf),
             "LOG=%d\nKEYS=%d\nEVENTS=%d\nDISPLAY=%d\nAUDIO=%d\nFPS=%d\n",
             s->log, s->log_keys, s->log_events, s->log_display, s->log_audio,
             s->show_fps);
    mkdirs(g_data);
    write_file(path, buf);
}

static int cfg_value(const char *path, const char *key, char *out, size_t n) {
    unsigned char *d;
    char *p;
    size_t klen = strlen(key);
    int found = 0;
    out[0] = 0;
    if (!read_file(path, &d, NULL)) return 0;
    p = (char *)d;
    while (*p) {
        char *e = strchr(p, '\n');
        if (e) *e = 0;
        if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
            char *v = p + klen + 1, *t = v + strlen(v);
            while (t > v && (t[-1] == '\r' || t[-1] == ' ')) *--t = 0;
            copy_str(out, n, v);
            found = 1;           /* the last line wins, as in run.sh */
        }
        if (!e) break;
        p = e + 1;
    }
    free(d);
    return found;
}

/* what the runtime menu wrote while the game ran: show it in the launcher */
static void absorb_runtime_cfg(Settings *s, const char *jar) {
    char path[BUF], stem[BUF], v[64];
    snprintf(path, sizeof(path), "%s/debug.cfg", g_data);
    if (cfg_value(path, "LOG", v, sizeof(v))) s->log = atoi(v) != 0;
    if (cfg_value(path, "KEYS", v, sizeof(v))) s->log_keys = atoi(v) != 0;
    if (cfg_value(path, "EVENTS", v, sizeof(v))) s->log_events = atoi(v) != 0;
    if (cfg_value(path, "DISPLAY", v, sizeof(v))) s->log_display = atoi(v) != 0;
    if (cfg_value(path, "AUDIO", v, sizeof(v))) s->log_audio = atoi(v) != 0;
    if (cfg_value(path, "FPS", v, sizeof(v))) s->show_fps = atoi(v) != 0;

    stem_of(jar, stem, sizeof(stem));
    if (!ascii_only(stem)) return;
    snprintf(path, sizeof(path), "%s/run/%s/game.cfg", g_data, stem);
    if (!is_file(path)) return;
    if (cfg_value(path, "DISPLAY_MODE", v, sizeof(v))) copy_str(s->mode, sizeof(s->mode), v);
    if (cfg_value(path, "FILTER", v, sizeof(v))) copy_str(s->filter, sizeof(s->filter), v);
    if (cfg_value(path, "FPS_LIMIT", v, sizeof(v))) copy_str(s->fps, sizeof(s->fps), v);
    if (cfg_value(path, "VOLUME", v, sizeof(v))) s->volume = atoi(v);
    if (cfg_value(path, "SYNTH", v, sizeof(v))) copy_str(s->synth, sizeof(s->synth), v);
    if (cfg_value(path, "VIEW", v, sizeof(v))) {
        if (strcmp(v, "full") == 0) s->lcd[0] = 0;
        else copy_str(s->lcd, sizeof(s->lcd), v);
    }
    if (cfg_value(path, "SCALE", v, sizeof(v)) && strncmp(v, "zoom-", 5) == 0) {
        int z = atoi(v + 5);
        s->zoom = strlen(v + 5) <= 3 ? z * 10 : z;
    }
    if (cfg_value(path, "PAN_X", v, sizeof(v))) {
        char y[64];
        cfg_value(path, "PAN_Y", y, sizeof(y));
        snprintf(s->pan, sizeof(s->pan), "%s,%s", v, y[0] ? y : "0");
        if (strcmp(s->pan, "0,0") == 0) s->pan[0] = 0;
    }
}

/* ===================================================================== */
/*  the game list                                                        */
/* ===================================================================== */

typedef struct {
    char path[BUF];
    char title[128];
    int doja;
} Game;

static Game g_games[MAX_GAMES];
static int g_count;

/* MIDlet-Name of the jar (or AppName of the .jam), else the file name */
static void game_title(Game *g) {
    char jam[BUF], stem[BUF], val[128];
    unsigned char *d;
    size_t len = strlen(g->path);

    stem_of(g->path, stem, sizeof(stem));
    copy_str(g->title, sizeof(g->title), stem);
    if (len < 5) return;

    snprintf(jam, sizeof(jam), "%.*s.jam", (int)(len - 4), g->path);
    if (!is_file(jam)) snprintf(jam, sizeof(jam), "%.*s.JAM", (int)(len - 4), g->path);
    if (is_file(jam) && read_file(jam, &d, NULL)) {
        g->doja = 1;
        if (desc_value((char *)d, "AppName", val, sizeof(val)))
            copy_str(g->title, sizeof(g->title), val);
        free(d);
        if (g->doja) return;
    }
    {
        Zip z;
        if (zip_open(&z, g->path)) {
            unsigned char *m = zip_read(&z, "META-INF/MANIFEST.MF", 1);
            if (m != NULL) {
                if (desc_value((char *)m, "MIDlet-Name", val, sizeof(val)))
                    copy_str(g->title, sizeof(g->title), val);
                free(m);
            }
            zip_close(&z);
        }
    }
}

static int game_cmp(const void *a, const void *b) {
    return strcasecmp(((const Game *)a)->title, ((const Game *)b)->title);
}

static void scan_dir(const char *dir, int depth) {
    DIR *d = opendir(dir);
    struct dirent *e;
    if (d == NULL) return;
    while ((e = readdir(d)) != NULL && g_count < MAX_GAMES) {
        char path[BUF];
        const char *dot;
        if (e->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        if (is_dir(path)) {
            if (depth < 4) scan_dir(path, depth + 1);
            continue;
        }
        dot = strrchr(e->d_name, '.');
        if (dot == NULL || strcasecmp(dot, ".jar") != 0) continue;
        copy_str(g_games[g_count].path, BUF, path);
        g_games[g_count].doja = 0;
        game_title(&g_games[g_count]);
        g_count++;
    }
    closedir(d);
}

static void scan_games(const char *dir) {
    g_count = 0;
    scan_dir(dir, 0);
    qsort(g_games, (size_t)g_count, sizeof(Game), game_cmp);
}

/* ===================================================================== */
/*  starting a game                                                      */
/* ===================================================================== */

static void env_int(const char *key, int v) {
    char s[32];
    snprintf(s, sizeof(s), "%d", v);
    setenv(key, s, 1);
}

static void video_init(void);
static void video_done(void);

/* run.sh does the rest: the .jad, DoJa, the restart loop, logging */
static int run_game(const Settings *s, const char *jar) {
    char keys[4096], run[BUF];
    pid_t pid;
    int status = 0;

    debug_cfg_write(s);
    snprintf(run, sizeof(run), "%s/run.sh", g_home);

    env_int("SCALE", s->scale);
    setenv("MODE", s->mode, 1);
    setenv("FILTER", s->filter, 1);
    setenv("FPS", s->fps[0] && strcmp(s->fps, "off") ? s->fps : "0", 1);
    if (s->lcd[0]) setenv("LCD", s->lcd, 1); else unsetenv("LCD");
    if (s->heap > 0) env_int("HEAP_MB", s->heap);
    if (s->mute) setenv("MUTE", "1", 1); else unsetenv("MUTE");
    env_int("VERIFY", s->verify);
    {
        char args[300];
        snprintf(args, sizeof(args), "%s%s%s", s->jit ? "" : "-UseCompiler",
                 (!s->jit && s->vmargs[0]) ? " " : "", s->vmargs);
        if (args[0]) setenv("VMARGS", args, 1); else unsetenv("VMARGS");
    }
    /* straight to the runtime: run.sh passes its environment on */
    setenv("PHONEME_FULLSCREEN", s->fullscreen ? "1" : "0", 1);
    env_int("PHONEME_SOUND_VOLUME", s->volume);
    setenv("PHONEME_SYNTH", s->synth, 1);
    if (s->zoom > 0) env_int("PHONEME_SCALE_RATIO", s->zoom);
    else unsetenv("PHONEME_SCALE_RATIO");
    if (s->pan[0]) setenv("PHONEME_PAN", s->pan, 1); else unsetenv("PHONEME_PAN");
    pc_keys_env(s, keys, sizeof(keys));
    setenv("PHONEME_PC_KEYS", keys, 1);

    pid = fork();
    if (pid == 0) {
        execl(run, run, jar, (char *)NULL);
        fprintf(stderr, "cannot start %s: %s\n", run, strerror(errno));
        _exit(127);
    }
    if (pid < 0) return -1;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) { }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* ===================================================================== */
/*  user interface                                                       */
/* ===================================================================== */

/* colours of the window, mapped to the surface in video_init() */
static Uint32 C_BG, C_PANEL, C_TEXT, C_DIM, C_SEL, C_ACCENT, C_TITLE;

static void colors_init(void) {
    C_BG     = SDL_MapRGB(scr->format, 0x18, 0x1c, 0x22);
    C_PANEL  = SDL_MapRGB(scr->format, 0x24, 0x2a, 0x33);
    C_TEXT   = SDL_MapRGB(scr->format, 0xdf, 0xe4, 0xec);
    C_DIM    = SDL_MapRGB(scr->format, 0x8d, 0x98, 0xa8);
    C_SEL    = SDL_MapRGB(scr->format, 0x3a, 0x6e, 0xa5);
    C_ACCENT = SDL_MapRGB(scr->format, 0x7f, 0xc4, 0xff);
    C_TITLE  = SDL_MapRGB(scr->format, 0xff, 0xff, 0xff);
}

enum { P_GAMES, P_DISPLAY, P_SOUND, P_KEYS, P_LOGS, P_COUNT };
static const char *PageName[P_COUNT] = { "Games", "Display", "Sound", "Controls", "Logs" };

static Settings g_set;          /* what the pages show */
static int g_page, g_sel[P_COUNT], g_top[P_COUNT];
static int g_game = -1;         /* selected game, -1 = none */
static int g_per_game;          /* the selected game has its own settings */
static int g_running = 1;
static int g_win_w = 940, g_win_h = 700;

/* editing state */
static int g_editing;           /* a text item is being typed into */
static char g_edit[BUF];
static int g_grab = -1;         /* waiting for a key for this bind slot */

typedef enum { IT_TOGGLE, IT_NUM, IT_OPT, IT_TEXT, IT_ACTION, IT_INFO } ItemType;

typedef struct {
    const char *label;
    ItemType type;
    void *val;                  /* int* or char* */
    size_t vsize;
    const char *const *ids;
    const char *const *names;
    int nopt;
    int min, max, step;
    const char *zero;           /* IT_NUM: name of the value 0 */
    const char *hint;
} Item;

static const char *const ModeIds[] = { "fit", "integer", "crop", "fill", "center", "zoom", "auto" };
static const char *const ModeNames[] = { "Fit (keep shape)", "Whole pixels", "Crop", "Stretch",
                                         "Centre 1:1", "Zoom", "Auto" };
static const char *const FilterIds[] = { "sharp", "scale2x", "smooth", "pixel" };
static const char *const FilterNames[] = { "Sharp", "Scale2x", "Smooth", "Pixel" };
static const char *const FpsIds[] = { "0", "20", "25", "30", "50", "60" };
static const char *const FpsNames[] = { "No limit", "20", "25", "30", "50", "60" };
static const char *const SynthIds[] = { "hybrid", "sonivox", "fm" };
static const char *const SynthNames[] = { "Hybrid (default)", "Sonivox (soundbank)", "FM" };

static int opt_index(const char *v, const char *const *ids, int n) {
    int i;
    for (i = 0; i < n; i++) if (strcasecmp(v, ids[i]) == 0) return i;
    return 0;
}

static Item ItemsDisplay[] = {
    { "Window size", IT_NUM, NULL, 0, NULL, NULL, 0, 1, 8, 1, NULL, "game screen x1..x8" },
    { "Full screen at start", IT_TOGGLE, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL, NULL },
    { "Picture in the window", IT_OPT, NULL, 0, ModeIds, ModeNames, 7, 0, 0, 0, NULL, NULL },
    { "Filter", IT_OPT, NULL, 0, FilterIds, FilterNames, 4, 0, 0, 0, NULL, NULL },
    { "Frame limit", IT_OPT, NULL, 0, FpsIds, FpsNames, 6, 0, 0, 0, NULL, NULL },
    { "Screen size the game sees", IT_TEXT, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL,
      "empty = as the game asks, else 240x320" },
    { "Zoom %", IT_NUM, NULL, 0, NULL, NULL, 0, 0, 400, 5, "off", "\"Zoom\" mode only" },
    { "Visible part (x,y)", IT_TEXT, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL,
      "zoom / crop: empty = centre" },
    { "Java heap, MB", IT_NUM, NULL, 0, NULL, NULL, 0, 0, 256, 8, "default", NULL },
    { "Compiler (JIT)", IT_TOGGLE, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL, NULL },
    { "Check bytecode", IT_TOGGLE, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL,
      "off: modded games that a phone would reject still run" },
    { "Extra VM options", IT_TEXT, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL, NULL }
};
#define N_DISPLAY ((int)(sizeof(ItemsDisplay) / sizeof(ItemsDisplay[0])))

static Item ItemsSound[] = {
    { "Sound", IT_TOGGLE, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL, NULL },
    { "Volume", IT_NUM, NULL, 0, NULL, NULL, 0, 0, 100, 5, NULL, NULL },
    { "MIDI synthesizer", IT_OPT, NULL, 0, SynthIds, SynthNames, 3, 0, 0, 0, NULL, NULL }
};
#define N_SOUND ((int)(sizeof(ItemsSound) / sizeof(ItemsSound[0])))

static Item ItemsLogs[] = {
    { "Log file", IT_TOGGLE, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL,
      "<data>/logs/<game>.log" },
    { "Log keys", IT_TOGGLE, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL, NULL },
    { "Log events", IT_TOGGLE, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL, NULL },
    { "Log display", IT_TOGGLE, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL, NULL },
    { "Log sound", IT_TOGGLE, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL, NULL },
    { "Show FPS in the game", IT_TOGGLE, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL, NULL },
    { "Saves, logs and settings are kept in", IT_INFO, NULL, 0, NULL, NULL, 0, 0, 0, 0, NULL, NULL }
};
#define N_LOGS ((int)(sizeof(ItemsLogs) / sizeof(ItemsLogs[0])))

/* the item values point into g_set, which the pages edit in place */
static void items_bind(void) {
    ItemsDisplay[0].val = &g_set.scale;
    ItemsDisplay[1].val = &g_set.fullscreen;
    ItemsDisplay[2].val = g_set.mode;      ItemsDisplay[2].vsize = sizeof(g_set.mode);
    ItemsDisplay[3].val = g_set.filter;    ItemsDisplay[3].vsize = sizeof(g_set.filter);
    ItemsDisplay[4].val = g_set.fps;       ItemsDisplay[4].vsize = sizeof(g_set.fps);
    ItemsDisplay[5].val = g_set.lcd;       ItemsDisplay[5].vsize = sizeof(g_set.lcd);
    ItemsDisplay[6].val = &g_set.zoom;
    ItemsDisplay[7].val = g_set.pan;       ItemsDisplay[7].vsize = sizeof(g_set.pan);
    ItemsDisplay[8].val = &g_set.heap;
    ItemsDisplay[9].val = &g_set.jit;
    ItemsDisplay[10].val = &g_set.verify;
    ItemsDisplay[11].val = g_set.vmargs;   ItemsDisplay[11].vsize = sizeof(g_set.vmargs);

    ItemsSound[0].val = &g_set.mute;       /* shown the other way round */
    ItemsSound[1].val = &g_set.volume;
    ItemsSound[2].val = g_set.synth;       ItemsSound[2].vsize = sizeof(g_set.synth);

    ItemsLogs[0].val = &g_set.log;
    ItemsLogs[1].val = &g_set.log_keys;
    ItemsLogs[2].val = &g_set.log_events;
    ItemsLogs[3].val = &g_set.log_display;
    ItemsLogs[4].val = &g_set.log_audio;
    ItemsLogs[5].val = &g_set.show_fps;
}

static Item *page_items(int page, int *n) {
    switch (page) {
    case P_DISPLAY: *n = N_DISPLAY; return ItemsDisplay;
    case P_SOUND:   *n = N_SOUND;   return ItemsSound;
    case P_LOGS:    *n = N_LOGS;    return ItemsLogs;
    }
    *n = 0;
    return NULL;
}

static void item_value(const Item *it, char *out, size_t n) {
    switch (it->type) {
    case IT_TOGGLE: {
        int v = *(int *)it->val;
        if (it->val == &g_set.mute) v = !v;      /* "Sound": on when not muted */
        copy_str(out, n, v ? "on" : "off");
        break;
    }
    case IT_NUM: {
        int v = *(int *)it->val;
        if (v == 0 && it->zero) copy_str(out, n, it->zero);
        else if (it->val == &g_set.scale) snprintf(out, n, "x%d", v);
        else snprintf(out, n, "%d", v);
        break;
    }
    case IT_OPT:
        copy_str(out, n, it->names[opt_index((char *)it->val, it->ids, it->nopt)]);
        break;
    case IT_TEXT:
        copy_str(out, n, ((char *)it->val)[0] ? (char *)it->val : "-");
        break;
    default:
        out[0] = 0;
    }
}

static void item_change(Item *it, int dir) {
    switch (it->type) {
    case IT_TOGGLE:
        *(int *)it->val = !*(int *)it->val;
        break;
    case IT_NUM: {
        int *v = (int *)it->val, step = it->step;
        if (it->val == &g_set.zoom && *v == 0 && dir > 0) { *v = 100; break; }
        *v += dir * step;
        if (*v < it->min) *v = it->min;
        if (*v > it->max) *v = it->max;
        break;
    }
    case IT_OPT: {
        int i = opt_index((char *)it->val, it->ids, it->nopt) + dir;
        if (i < 0) i = it->nopt - 1;
        if (i >= it->nopt) i = 0;
        copy_str((char *)it->val, it->vsize, it->ids[i]);
        break;
    }
    default:
        break;
    }
}

/* -------------------------------------------------------------- drawing */

static void draw_tabs(int y) {
    int i, x = 10 * UI;
    for (i = 0; i < P_COUNT; i++) {
        int w = text_w(PageName[i]) + 8 * UI;
        if (i == g_page) {
            fill(x, y - 2 * UI, w, LINE + 2 * UI, C_PANEL);
            fill(x, y + LINE - UI, w, UI, C_ACCENT);
        }
        text(x + 4 * UI, y, PageName[i], i == g_page ? C_TITLE : C_DIM);
        x += w + 2 * UI;
    }
}

/* the longest of the two hints that still fits the window */
static void draw_footer(const char *full, const char *shrt) {
    int y = g_win_h - LINE - 2 * UI;
    const char *s = text_w(full) <= g_win_w - 20 * UI ? full : shrt;
    fill(0, y - 2 * UI, g_win_w, LINE + 4 * UI, C_PANEL);
    text(10 * UI, y, s, C_DIM);
}

static void draw_games(int y0) {
    int rows = (g_win_h - y0 - 4 * LINE) / LINE, i;
    char line[256];

    if (rows < 1) rows = 1;
    if (g_sel[P_GAMES] < g_top[P_GAMES]) g_top[P_GAMES] = g_sel[P_GAMES];
    if (g_sel[P_GAMES] >= g_top[P_GAMES] + rows) g_top[P_GAMES] = g_sel[P_GAMES] - rows + 1;

    if (g_count == 0) {
        text(10 * UI, y0, "No .jar games in this folder.", C_DIM);
        text(10 * UI, y0 + LINE, "Press G to pick another one.", C_DIM);
    }
    for (i = 0; i < rows && g_top[P_GAMES] + i < g_count; i++) {
        int idx = g_top[P_GAMES] + i, y = y0 + i * LINE;
        Game *g = &g_games[idx];
        if (idx == g_sel[P_GAMES]) fill(6 * UI, y - UI, g_win_w - 12 * UI, LINE, C_SEL);
        text_fit(10 * UI, y, g->title, g_win_w - 24 * UI - 5 * GW,
                 idx == g_sel[P_GAMES] ? C_TITLE : C_TEXT);
        if (g->doja) text(g_win_w - 14 * UI - 4 * GW, y, "DoJa", C_ACCENT);
    }
    {
        int y = g_win_h - 3 * LINE - 2 * UI;
        if (g_editing == 2) {
            snprintf(line, sizeof(line), "Folder: %s_", g_edit);
            text_fit(10 * UI, y, line, g_win_w - 20 * UI, C_ACCENT);
        } else {
            snprintf(line, sizeof(line), "Folder: %s", g_set.games_dir);
            text_fit(10 * UI, y, line, g_win_w - 20 * UI, C_DIM);
        }
        if (g_count > 0 && g_sel[P_GAMES] < g_count) {
            snprintf(line, sizeof(line), "%s%s", g_per_game ? "Own settings - " : "",
                     g_games[g_sel[P_GAMES]].path);
            text_fit(10 * UI, y + LINE, line, g_win_w - 20 * UI,
                     g_per_game ? C_ACCENT : C_DIM);
        }
    }
}

static void draw_items(int y0, int page) {
    int n, i, rows = (g_win_h - y0 - 3 * LINE) / LINE;
    Item *items = page_items(page, &n);
    int labelw = 0;

    for (i = 0; i < n; i++) {
        int w = text_w(items[i].label);
        if (w > labelw) labelw = w;
    }
    if (rows < 1) rows = 1;
    if (g_sel[page] < g_top[page]) g_top[page] = g_sel[page];
    if (g_sel[page] >= g_top[page] + rows) g_top[page] = g_sel[page] - rows + 1;

    for (i = 0; i < rows && g_top[page] + i < n; i++) {
        int idx = g_top[page] + i, y = y0 + i * LINE;
        char val[128];
        Item *it = &items[idx];
        if (idx == g_sel[page]) fill(6 * UI, y - UI, g_win_w - 12 * UI, LINE, C_SEL);
        text(10 * UI, y, it->label, idx == g_sel[page] ? C_TITLE : C_TEXT);
        if (it->type == IT_INFO) continue;
        if (g_editing && idx == g_sel[page]) {
            snprintf(val, sizeof(val), "%s_", g_edit);
            text(14 * UI + labelw, y, val, C_ACCENT);
        } else {
            item_value(it, val, sizeof(val));
            text(14 * UI + labelw, y, val, idx == g_sel[page] ? C_TITLE : C_ACCENT);
        }
    }
    {
        Item *it = &items[g_sel[page]];
        if (it->hint) text(10 * UI, g_win_h - 2 * LINE - 2 * UI, it->hint, C_DIM);
        else if (it->type == IT_INFO) {
            char s[BUF];
            snprintf(s, sizeof(s), "%s", g_data);
            text(10 * UI, g_win_h - 2 * LINE - 2 * UI, s, C_DIM);
        }
    }
}

static void draw_keys(int y0) {
    int rows = (g_win_h - y0 - 3 * LINE) / LINE, i, j;
    int total = ACTION_COUNT + 1;      /* first row: the phone type */

    if (rows < 1) rows = 1;
    if (g_sel[P_KEYS] < g_top[P_KEYS]) g_top[P_KEYS] = g_sel[P_KEYS];
    if (g_sel[P_KEYS] >= g_top[P_KEYS] + rows) g_top[P_KEYS] = g_sel[P_KEYS] - rows + 1;

    for (i = 0; i < rows && g_top[P_KEYS] + i < total; i++) {
        int idx = g_top[P_KEYS] + i, y = y0 + i * LINE;
        int sel = idx == g_sel[P_KEYS];
        if (sel) fill(6 * UI, y - UI, g_win_w - 12 * UI, LINE, C_SEL);
        if (idx == 0) {
            text(10 * UI, y, "Soft keys of the phone", sel ? C_TITLE : C_TEXT);
            text(14 * UI + 24 * GW, y, Phones[phone_index(g_set.phone)].label,
                 sel ? C_TITLE : C_ACCENT);
            continue;
        }
        {
            int a = idx - 1, x = 14 * UI + 24 * GW;
            char name[32];
            text(10 * UI, y, Actions[a].label, sel ? C_TITLE : C_TEXT);
            for (j = 0; j < MAX_BIND; j++) {
                const char *nm;
                if (sel && g_grab == j) nm = "[press]";
                else if (g_set.keys[a][j]) {
                    copy_str(name, sizeof(name), SDL_GetKeyName((SDLKey)g_set.keys[a][j]));
                    name[0] = (char)toupper((unsigned char)name[0]);
                    nm = name;
                }
                else nm = "-";
                text(x, y, nm, sel && g_grab == j ? C_TITLE :
                               (g_set.keys[a][j] ? (sel ? C_TITLE : C_ACCENT) : C_DIM));
                x += 11 * GW;
            }
        }
    }
    text(10 * UI, g_win_h - 2 * LINE - 2 * UI,
         g_sel[P_KEYS] == 0 ? "Left / Right: the key codes the game gets"
                            : "Enter: set a key   Del: clear   1/2/3: which of the three",
         C_DIM);
}

static void draw(void) {
    int y0;

    fill(0, 0, g_win_w, g_win_h, C_BG);
    fill(0, 0, g_win_w, 3 * LINE, C_PANEL);
    text(10 * UI, LINE / 3, "phoneME", C_TITLE);
    text(10 * UI + 8 * GW, LINE / 3, "J2ME and i-appli games", C_DIM);
    draw_tabs(LINE + LINE / 3);
    y0 = 3 * LINE + LINE / 2;

    switch (g_page) {
    case P_GAMES:  draw_games(y0); break;
    case P_KEYS:   draw_keys(y0); break;
    default:       draw_items(y0, g_page); break;
    }

    if (g_editing) {
        draw_footer("Type the path, then Enter; Esc leaves it as it was",
                    "Enter: use it   Esc: cancel");
    } else if (g_page == P_GAMES) {
        draw_footer("Enter: play   G: folder   O: own settings   R: rescan   "
                    "Tab: page   Esc: quit",
                    "Enter: play   G: folder   R: rescan   Esc: quit");
    } else if (g_page == P_KEYS) {
        draw_footer("Enter: set a key   Del: clear   1/2/3: which of the three   "
                    "Tab: page   Esc: quit",
                    "Enter: set   Del: clear   Tab: page   Esc: quit");
    } else {
        draw_footer("Left / Right: change   Enter: change or type   "
                    "Tab: page   Esc: quit",
                    "Left / Right: change   Tab: page   Esc: quit");
    }
    SDL_Flip(scr);
}

/* ---------------------------------------------------------------- input */

static void save_current(void) {
    char gi[BUF];
    if (g_per_game && g_game >= 0) {
        game_ini_path(g_games[g_game].path, gi, sizeof(gi));
        if (gi[0]) {
            char dir[BUF];
            snprintf(dir, sizeof(dir), "%s/games", g_data);
            mkdirs(dir);
            settings_save_file(&g_set, gi, 0);
        }
    } else {
        mkdirs(g_data);
        settings_save_file(&g_set, g_ini, 1);
    }
    debug_cfg_write(&g_set);
}

/* the settings of the selected game (its own ones when it has any) */
static void select_game(int idx) {
    char gi[BUF], games_dir[BUF];
    copy_str(games_dir, sizeof(games_dir), g_set.games_dir);
    g_game = idx;
    g_per_game = 0;
    if (idx >= 0 && idx < g_count) {
        game_ini_path(g_games[idx].path, gi, sizeof(gi));
        if (gi[0] && is_file(gi)) g_per_game = 1;
        settings_for(&g_set, g_games[idx].path);
    } else {
        settings_for(&g_set, NULL);
    }
    copy_str(g_set.games_dir, sizeof(g_set.games_dir), games_dir);
    items_bind();
}

static void play(int idx) {
    char jar[BUF];
    if (idx < 0 || idx >= g_count) return;
    copy_str(jar, sizeof(jar), g_games[idx].path);
    save_current();
    video_done();
    printf("\n--- %s\n", jar);
    fflush(stdout);
    run_game(&g_set, jar);
    absorb_runtime_cfg(&g_set, jar);
    save_current();
    video_init();
}

static void start_edit(const char *cur) {
    g_editing = 1;
    copy_str(g_edit, sizeof(g_edit), cur);
}

static void finish_edit(int keep) {
    int n;
    Item *items = page_items(g_page, &n);
    g_editing = 0;
    if (!keep || items == NULL) return;
    {
        Item *it = &items[g_sel[g_page]];
        if (it->type == IT_TEXT) copy_str((char *)it->val, it->vsize, g_edit);
    }
}

static void key_pressed(SDL_Event *e) {
    SDLKey k = e->key.keysym.sym;
    int n = 0;
    Item *items = page_items(g_page, &n);

    if (g_editing) {
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER) finish_edit(1);
        else if (k == SDLK_ESCAPE) finish_edit(0);
        else if (k == SDLK_BACKSPACE) {
            size_t l = strlen(g_edit);
            if (l) g_edit[l - 1] = 0;
        } else if (e->key.keysym.unicode >= 32 && e->key.keysym.unicode < 127) {
            size_t l = strlen(g_edit);
            if (l + 1 < sizeof(g_edit)) { g_edit[l] = (char)e->key.keysym.unicode; g_edit[l + 1] = 0; }
        }
        return;
    }
    if (g_grab >= 0) {                      /* waiting for a key to bind */
        if (k != SDLK_ESCAPE && g_sel[P_KEYS] > 0)
            g_set.keys[g_sel[P_KEYS] - 1][g_grab] = (int)k;
        g_grab = -1;
        return;
    }

    switch (k) {
    case SDLK_ESCAPE:
        g_running = 0;
        return;
    case SDLK_TAB:
        g_page = (e->key.keysym.mod & KMOD_SHIFT) ? (g_page + P_COUNT - 1) % P_COUNT
                                                  : (g_page + 1) % P_COUNT;
        return;
    case SDLK_UP:
        if (g_sel[g_page] > 0) g_sel[g_page]--;
        if (g_page == P_GAMES) select_game(g_sel[P_GAMES]);
        return;
    case SDLK_DOWN: {
        int max = g_page == P_GAMES ? g_count - 1
                : g_page == P_KEYS ? ACTION_COUNT
                : n - 1;
        if (g_sel[g_page] < max) g_sel[g_page]++;
        if (g_page == P_GAMES) select_game(g_sel[P_GAMES]);
        return;
    }
    case SDLK_PAGEUP:
    case SDLK_PAGEDOWN: {
        int step = 8, max = g_page == P_GAMES ? g_count - 1
                          : g_page == P_KEYS ? ACTION_COUNT : n - 1;
        g_sel[g_page] += (k == SDLK_PAGEUP ? -step : step);
        if (g_sel[g_page] < 0) g_sel[g_page] = 0;
        if (g_sel[g_page] > max) g_sel[g_page] = max > 0 ? max : 0;
        if (g_page == P_GAMES) select_game(g_sel[P_GAMES]);
        return;
    }
    default:
        break;
    }

    if (g_page == P_GAMES) {
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER) play(g_sel[P_GAMES]);
        else if (k == 'g') { start_edit(g_set.games_dir); g_editing = 2; }
        else if (k == 'r') { scan_games(g_set.games_dir); g_sel[P_GAMES] = 0; select_game(0); }
        else if (k == 'o' && g_game >= 0) {
            char gi[BUF];
            game_ini_path(g_games[g_game].path, gi, sizeof(gi));
            if (gi[0]) {
                if (g_per_game) { remove(gi); g_per_game = 0; select_game(g_game); }
                else { g_per_game = 1; save_current(); }
            }
        }
        return;
    }
    if (g_page == P_KEYS) {
        if (g_sel[P_KEYS] == 0) {
            if (k == SDLK_LEFT || k == SDLK_RIGHT) {
                int i = phone_index(g_set.phone) + (k == SDLK_RIGHT ? 1 : -1);
                if (i < 0) i = PHONE_COUNT - 1;
                if (i >= PHONE_COUNT) i = 0;
                copy_str(g_set.phone, sizeof(g_set.phone), Phones[i].id);
            }
            return;
        }
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER) g_grab = 0;
        else if (k >= '1' && k <= '3') g_grab = k - '1';
        else if (k == SDLK_DELETE || k == SDLK_BACKSPACE) {
            int a = g_sel[P_KEYS] - 1, j;
            for (j = MAX_BIND - 1; j >= 0; j--)
                if (g_set.keys[a][j]) { g_set.keys[a][j] = 0; break; }
        }
        return;
    }
    if (items == NULL) return;
    {
        Item *it = &items[g_sel[g_page]];
        if (k == SDLK_LEFT) item_change(it, -1);
        else if (k == SDLK_RIGHT) item_change(it, 1);
        else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
            if (it->type == IT_TEXT) start_edit((char *)it->val);
            else item_change(it, 1);
        }
    }
}

static void mouse_pressed(SDL_Event *e) {
    int y0 = 3 * LINE + LINE / 2, i, x = 10 * UI, ty = LINE + LINE / 3;
    if (e->button.y >= ty - 2 * UI && e->button.y < ty + LINE) {
        for (i = 0; i < P_COUNT; i++) {
            int w = text_w(PageName[i]) + 8 * UI;
            if (e->button.x >= x && e->button.x < x + w) { g_page = i; return; }
            x += w + 2 * UI;
        }
        return;
    }
    if (e->button.button == SDL_BUTTON_WHEELUP || e->button.button == SDL_BUTTON_WHEELDOWN) {
        SDL_Event k;
        memset(&k, 0, sizeof(k));
        k.key.keysym.sym = e->button.button == SDL_BUTTON_WHEELUP ? SDLK_UP : SDLK_DOWN;
        key_pressed(&k);
        return;
    }
    if (e->button.y < y0) return;
    {
        int row = (e->button.y - y0) / LINE, idx = g_top[g_page] + row, max;
        max = g_page == P_GAMES ? g_count - 1 : g_page == P_KEYS ? ACTION_COUNT : 0;
        if (g_page != P_GAMES && g_page != P_KEYS) {
            int n;
            page_items(g_page, &n);
            max = n - 1;
        }
        if (idx < 0 || idx > max) return;
        if (g_page == P_GAMES) {
            if (idx == g_sel[P_GAMES]) { play(idx); return; }
            g_sel[P_GAMES] = idx;
            select_game(idx);
        } else {
            g_sel[g_page] = idx;
        }
    }
}

/* ===================================================================== */
/*  window                                                               */
/* ===================================================================== */

static void video_init(void) {
    const SDL_VideoInfo *vi;
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL video: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_WM_SetCaption("phoneME J2ME", "phoneME");
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    vi = SDL_GetVideoInfo();
    if (vi != NULL && vi->current_w > 0) {
        if (g_win_w > vi->current_w - 40) g_win_w = vi->current_w - 40;
        if (g_win_h > vi->current_h - 80) g_win_h = vi->current_h - 80;
    }
    if (g_win_w < 420) g_win_w = 420;
    if (g_win_h < 320) g_win_h = 320;
    scr = SDL_SetVideoMode(g_win_w, g_win_h, 32, SDL_SWSURFACE | SDL_RESIZABLE);
    if (scr == NULL) {
        fprintf(stderr, "SDL window: %s\n", SDL_GetError());
        exit(1);
    }
    UI = g_win_w < 560 ? 1 : (g_win_w < 1500 ? 2 : 3);
    colors_init();
}

static void video_done(void) {
    scr = NULL;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

/* ===================================================================== */

static void paths_init(const char *argv0) {
    char exe[BUF];
    const char *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    char *slash;

    if (n > 0) exe[n] = 0;
    else copy_str(exe, sizeof(exe), argv0);
    {   /* realpath() wants a PATH_MAX buffer */
        char full[PATH_MAX];
        if (realpath(exe, full) != NULL) copy_str(g_home, sizeof(g_home), full);
        else copy_str(g_home, sizeof(g_home), exe);
    }
    slash = strrchr(g_home, '/');
    if (slash) *slash = 0;

    if (xdg && xdg[0]) snprintf(g_data, sizeof(g_data), "%s/phoneme-j2me", xdg);
    else snprintf(g_data, sizeof(g_data), "%s/.local/share/phoneme-j2me", home ? home : ".");
    snprintf(g_ini, sizeof(g_ini), "%s/settings.ini", g_data);
}

int main(int argc, char **argv) {
    int i;

    paths_init(argv[0]);
    settings_for(&g_set, NULL);
    items_bind();

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("phoneME: J2ME / i-appli games\n"
                   "  phoneME             game list and settings\n"
                   "  phoneME game.jar    start the game\n"
                   "Settings: %s\n", g_ini);
            return 0;
        }
        if (is_file(argv[i])) {           /* start this game right away */
            char jar[BUF], full[PATH_MAX];
            if (realpath(argv[i], full) != NULL) copy_str(jar, sizeof(jar), full);
            else copy_str(jar, sizeof(jar), argv[i]);
            settings_for(&g_set, jar);
            return run_game(&g_set, jar) != 0;
        }
        fprintf(stderr, "No such file: %s\n", argv[i]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
    video_init();

    mkdirs(g_set.games_dir);
    scan_games(g_set.games_dir);
    select_game(g_count > 0 ? 0 : -1);

    while (g_running) {
        SDL_Event e;
        draw();
        if (SDL_WaitEvent(&e) == 0) break;
        do {
            switch (e.type) {
            case SDL_QUIT:
                g_running = 0;
                break;
            case SDL_VIDEORESIZE:
                g_win_w = e.resize.w;
                g_win_h = e.resize.h;
                video_done();
                video_init();
                break;
            case SDL_KEYDOWN:
                /* the games folder is typed in on the games page */
                if (g_editing == 2) {
                    SDLKey k = e.key.keysym.sym;
                    if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
                        expand_path(g_edit, g_set.games_dir, BUF);
                        g_editing = 0;
                        mkdirs(g_set.games_dir);
                        scan_games(g_set.games_dir);
                        g_sel[P_GAMES] = 0;
                        select_game(g_count > 0 ? 0 : -1);
                        mkdirs(g_data);
                        settings_save_file(&g_set, g_ini, 1);
                    } else if (k == SDLK_ESCAPE) {
                        g_editing = 0;
                    } else if (k == SDLK_BACKSPACE) {
                        size_t l = strlen(g_edit);
                        if (l) g_edit[l - 1] = 0;
                    } else if (e.key.keysym.unicode >= 32 && e.key.keysym.unicode < 127) {
                        size_t l = strlen(g_edit);
                        if (l + 1 < sizeof(g_edit)) {
                            g_edit[l] = (char)e.key.keysym.unicode;
                            g_edit[l + 1] = 0;
                        }
                    }
                    break;
                }
                key_pressed(&e);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (!g_editing) mouse_pressed(&e);
                break;
            default:
                break;
            }
        } while (g_running && SDL_PollEvent(&e));
    }

    save_current();
    video_done();
    SDL_Quit();
    return 0;
}
