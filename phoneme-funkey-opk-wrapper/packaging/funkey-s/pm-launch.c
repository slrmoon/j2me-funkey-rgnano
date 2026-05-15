#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <SDL_gfxPrimitives_font.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* ── screen ─────────────────────────────────────────── */
#define SCR_W  240
#define SCR_H  240

/* ── directories ────────────────────────────────────── */
#define PM_DIR     "/mnt/FunKey/.pm"
#define BIND_DIR   "/mnt/FunKey/.pm/keybinds"
#define BIN_DIR    "/mnt/FunKey/.pm/bin"
static char java_dir[PATH_MAX] = "/mnt/java";

/* ── MIDP key codes (from phoneME keymap_input.h) ───── */
#define K_UP     (-1)
#define K_DOWN   (-2)
#define K_LEFT   (-3)
#define K_RIGHT  (-4)
#define K_SELECT (-5)
#define K_SOFT1  (-6)
#define K_SOFT2  (-7)
#define K_CLEAR  (-8)
#define K_GAMEA  (-13)
#define K_GAMEB  (-14)
#define K_GAMEC  (-15)
#define K_GAMED  (-16)
#define K_SEND   (-10)
#define K_END    (-11)
#define K_POWER  (-12)
#define K_GAME_UP    (-17)
#define K_GAME_DOWN  (-18)
#define K_GAME_LEFT  (-19)
#define K_GAME_RIGHT (-20)
/* raw ASCII: '*'=42 '#'=35 '5'=53 */
static const char *bind_names[] = {
    "UP", "DOWN", "LEFT", "RIGHT",
    "A", "B", "X", "Y",
    "L1", "R1",
    "START", "SELECT",
    NULL
};

static const char *bind_labels[] = {
    "FK UP", "FK DOWN", "FK LEFT", "FK RIGHT",
    "FK A", "FK B", "FK X", "FK Y",
    "FK L1", "FK R1",
    "FK START", "FK SELECT",
    NULL
};

static const char *bind_gp2x_env[] = {
    "J2ME_GP2X_JOYU", "J2ME_GP2X_JOYD",
    "J2ME_GP2X_JOYL", "J2ME_GP2X_JOYR",
    "J2ME_GP2X_BUTA", "J2ME_GP2X_BUTB",
    "J2ME_GP2X_BUTX", "J2ME_GP2X_BUTY",
    "J2ME_GP2X_LEFT", "J2ME_GP2X_RIGHT",
    "J2ME_GP2X_START", "J2ME_GP2X_SELECT",
    NULL
};

/* default MIDP key codes for FunKey S (KEYMAP.md) */
static const int bind_defaults[] = {
    -1, -2, -3, -4,       /* UP, DOWN, LEFT, RIGHT */
    -5, -14, -15, -16,     /* A, B, X, Y */
    42, 35,                 /* L1, R1 */
    -7, -6                  /* START→SOFT2, SELECT→SOFT1 */
};

#define BIND_COUNT  ((int)(sizeof(bind_defaults) / sizeof(bind_defaults[0])))
#define BIND_ROW_OFFSET 4

typedef struct {
    const char *id;
    const char *label;
    int code;
} PhoneKeyOption;

typedef struct {
    const char *id;
    const char *label;
    int defaults[BIND_COUNT];
} KeyProfilePreset;

static const PhoneKeyOption phone_key_options[] = {
    { "up", "PHONE UP", K_UP },
    { "down", "PHONE DOWN", K_DOWN },
    { "left", "PHONE LEFT", K_LEFT },
    { "right", "PHONE RIGHT", K_RIGHT },
    { "fire", "PHONE FIRE", K_SELECT },
    { "soft1", "SOFT L NOK -6", K_SOFT1 },
    { "soft2", "SOFT R NOK -7", K_SOFT2 },
    { "soft_l_21", "SOFT L 21", 21 },
    { "soft_r_22", "SOFT R 22", 22 },
    { "soft_l_moto", "SOFT L MOTO -21", -21 },
    { "soft_r_moto", "SOFT R MOTO -22", -22 },
    { "clear", "PHONE CLEAR", K_CLEAR },
    { "send", "PHONE SEND", K_SEND },
    { "end", "PHONE END", K_END },
    { "gamea", "PHONE GAMEA", K_GAMEA },
    { "gameb", "PHONE GAMEB", K_GAMEB },
    { "gamec", "PHONE GAMEC", K_GAMEC },
    { "gamed", "PHONE GAMED", K_GAMED },
    { "gameup", "PHONE GAME UP", K_GAME_UP },
    { "gamedown", "PHONE GAME DOWN", K_GAME_DOWN },
    { "gameleft", "PHONE GAME LEFT", K_GAME_LEFT },
    { "gameright", "PHONE GAME RIGHT", K_GAME_RIGHT },
    { "1", "PHONE 1", 49 },
    { "2", "PHONE 2", 50 },
    { "3", "PHONE 3", 51 },
    { "4", "PHONE 4", 52 },
    { "5", "PHONE 5", 53 },
    { "6", "PHONE 6", 54 },
    { "7", "PHONE 7", 55 },
    { "8", "PHONE 8", 56 },
    { "9", "PHONE 9", 57 },
    { "0", "PHONE 0", 48 },
    { "star", "PHONE *", 42 },
    { "pound", "PHONE #", 35 }
};

#define PHONE_KEY_COUNT ((int)(sizeof(phone_key_options) / sizeof(phone_key_options[0])))

static const KeyProfilePreset key_profiles[] = {
    { "universal", "UNIVERSAL",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, K_GAMEB, K_GAMEC, K_GAMED,
        42, 35, K_SOFT2, K_SOFT1 } },
    { "nokia-s40", "NOKIA/S40",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, 53, K_SOFT1, K_SOFT2,
        42, 35, K_SOFT2, K_SOFT1 } },
    { "sony-ericsson", "SONY ERICSSON",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, 53, K_SOFT2, K_SOFT1,
        K_CLEAR, 35, K_SOFT1, K_SOFT2 } },
    { "samsung", "SAMSUNG",
      { 50, 56, 52, 54, 53, K_SELECT, K_SOFT1, K_SOFT2,
        42, 35, K_SOFT2, K_SOFT1 } },
    { "lg", "LG",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, 53, K_CLEAR, K_SOFT2,
        42, 35, K_SOFT1, K_SOFT2 } },
    { "motorola", "MOTOROLA",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, K_GAMEA, K_SOFT1, K_SOFT2,
        K_SEND, K_END, K_SOFT2, K_SOFT1 } },
    { "soft-21-22", "SOFT 21/22",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, 53, K_GAMEC, K_GAMED,
        42, 35, 22, 21 } },
    { "motorola-soft", "MOTO SOFT -21/-22",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, K_GAMEA, K_GAMEC, K_GAMED,
        K_SEND, K_END, -22, -21 } },
    { "siemens-soft", "SIEMENS SOFT -1/-4",
      { 50, 56, 52, 54, 53, K_SELECT, K_GAMEC, K_GAMED,
        42, 35, -4, -1 } }
};

#define KEY_PROFILE_COUNT ((int)(sizeof(key_profiles) / sizeof(key_profiles[0])))

typedef struct {
    const char *id;
    const char *label;
    const char *scale_mode;
    int ratio;
} ScalePreset;

typedef struct {
    const char *id;
    const char *label;
    int width;
    int height;
} SourcePreset;

static const ScalePreset scale_presets[] = {
    { "zoom-075", "x0.75", "zoom", 750 },
    { "zoom-100", "x1.00", "zoom", 1000 },
    { "zoom-120", "x1.20", "zoom", 1200 },
    { "zoom-140", "x1.40", "zoom", 1400 },
    { "zoom-150", "x1.50", "zoom", 1500 },
    { "zoom-160", "x1.60", "zoom", 1600 },
    { "zoom-170", "x1.70", "zoom", 1700 },
    { "zoom-180", "x1.80", "zoom", 1800 },
    { "zoom-185", "x1.85", "zoom", 1850 }
};

#define SCALE_COUNT  ((int)(sizeof(scale_presets) / sizeof(scale_presets[0])))

static const ScalePreset scale_128x128[] = {
    { "zoom-150", "x1.50", "zoom", 1500 },
    { "zoom-175", "x1.75", "zoom", 1750 },
    { "zoom-1875", "x1.875", "zoom", 1875 },
    { "zoom-200", "x2.00", "zoom", 2000 }
};

static const ScalePreset scale_128x160[] = {
    { "zoom-140", "x1.40", "zoom", 1400 },
    { "zoom-150", "x1.50", "zoom", 1500 },
    { "zoom-160", "x1.60", "zoom", 1600 },
    { "zoom-170", "x1.70", "zoom", 1700 }
};

static const ScalePreset scale_176x208[] = {
    { "zoom-100", "x1.00", "zoom", 1000 },
    { "zoom-110", "x1.10", "zoom", 1100 },
    { "zoom-115", "x1.15", "zoom", 1150 },
    { "zoom-120", "x1.20", "zoom", 1200 }
};

static const ScalePreset scale_176x220[] = {
    { "zoom-100", "x1.00", "zoom", 1000 },
    { "zoom-105", "x1.05", "zoom", 1050 },
    { "zoom-109", "x1.09", "zoom", 1090 },
    { "zoom-115", "x1.15", "zoom", 1150 },
    { "zoom-120", "x1.20", "zoom", 1200 }
};

static const ScalePreset scale_208x208[] = {
    { "zoom-100", "x1.00", "zoom", 1000 },
    { "zoom-110", "x1.10", "zoom", 1100 },
    { "zoom-115", "x1.15", "zoom", 1150 }
};

static const ScalePreset scale_240x320[] = {
    { "zoom-075", "x0.75", "zoom", 750 },
    { "zoom-080", "x0.80", "zoom", 800 },
    { "zoom-085", "x0.85", "zoom", 850 },
    { "zoom-090", "x0.90", "zoom", 900 },
    { "zoom-100", "x1.00", "zoom", 1000 }
};

static const ScalePreset scale_320x240[] = {
    { "zoom-075", "x0.75", "zoom", 750 },
    { "zoom-080", "x0.80", "zoom", 800 },
    { "zoom-085", "x0.85", "zoom", 850 },
    { "zoom-090", "x0.90", "zoom", 900 },
    { "zoom-100", "x1.00", "zoom", 1000 }
};

static const SourcePreset source_presets[] = {
    { "auto", "AUTO", 0, 0 },
    { "full", "FULL", 0, 0 },
    { "128x128", "128x128", 128, 128 },
    { "128x160", "128x160", 128, 160 },
    { "176x208", "176x208", 176, 208 },
    { "176x220", "176x220", 176, 220 },
    { "208x208", "208x208", 208, 208 },
    { "240x320", "240x320", 240, 320 },
    { "320x240", "320x240", 320, 240 }
};

#define SOURCE_COUNT  ((int)(sizeof(source_presets) / sizeof(source_presets[0])))

static const char *display_modes[] = {
    "auto", "fit", "fill", "crop", "center", "zoom"
};

#define DISPLAY_MODE_COUNT ((int)(sizeof(display_modes) / sizeof(display_modes[0])))

/* ── game list ──────────────────────────────────────── */
#define MAX_GAMES   512
static char  *games[MAX_GAMES];
static int    game_count = 0;
static int    game_sel   = 0;        /* selected index in browser */
static int    game_scroll = 0;       /* first visible line */

/* ── current key bindings (loaded / defaults) ───────── */
static int    binds[BIND_COUNT];
static int    scale_sel = 0;
static int    source_sel = 0;
static int    display_mode_sel = 5;
static int    sound_volume = 100;
static int    key_profile_sel = 0;
static int    bind_sel   = 0;        /* selected row in settings */
static int    bind_scroll = 0;
static int    capture_mode = 0;       /* 1 = waiting for key press to bind */

/* ── state ──────────────────────────────────────────── */
enum { STATE_BROWSER, STATE_KEYBINDS, STATE_LAUNCHING };
static int state = STATE_BROWSER;

/* ── SDL objects ────────────────────────────────────── */
static SDL_Surface *screen = NULL;

/* ── colours ───────────────────────────────────────── */
static SDL_Color clr_bg     = {0x10, 0x18, 0x20, 0};
static SDL_Color clr_title  = {0x7d, 0xf9, 0xff, 0};
static SDL_Color clr_text   = {0xff, 0xff, 0xff, 0};
static SDL_Color clr_sel    = {0x00, 0xff, 0x88, 0};
static SDL_Color clr_dim    = {0x60, 0x70, 0x78, 0};
static SDL_Color clr_warn   = {0xff, 0x60, 0x60, 0};

/* ── forward decls ──────────────────────────────────── */
static void scan_games(void);
static void load_binds(const char *game_name, const char *jar_path);
static void save_binds(const char *game_name);
static void launch_game(int idx);
static void draw_browser(void);
static void draw_keybinds(void);
static void draw_launching(void);
static void handle_key_browser(SDL_KeyboardEvent *kev);
static void handle_key_keybinds(SDL_KeyboardEvent *kev);

/* ─────────────────────────────────────────────────────
 *  game scanning
 * ──────────────────────────────────────────────────── */
static void scan_games(void) {
    DIR *d;
    struct dirent *ent;
    int i;

    for (i = 0; i < game_count; i++) free(games[i]);
    game_count = 0;
    game_sel = 0;
    game_scroll = 0;

    d = opendir(java_dir);
    if (!d) return;

    while ((ent = readdir(d)) != NULL) {
        const char *n = ent->d_name;
        int len = strlen(n);
        if (len < 5) continue;
        /* case-insensitive .jar suffix */
        if (strcasecmp(n + len - 4, ".jar") != 0) continue;
        if (game_count >= MAX_GAMES) break;

        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", java_dir, n);
        games[game_count] = strdup(full);
        game_count++;
    }
    closedir(d);

    /* simple sort */
    for (i = 0; i < game_count - 1; i++) {
        int j;
        for (j = i + 1; j < game_count; j++) {
            if (strcasecmp(games[i], games[j]) > 0) {
                char *tmp = games[i];
                games[i] = games[j];
                games[j] = tmp;
            }
        }
    }
}

/* ─────────────────────────────────────────────────────
 *  key binding config read / write
 * ──────────────────────────────────────────────────── */
static const char *game_stem(const char *path) {
    const char *s = strrchr(path, '/');
    if (s) s++; else s = path;
    return s;
}

static char *trim_line(char *text) {
    char *end;
    while (*text == ' ' || *text == '\t') text++;
    end = text + strlen(text);
    while (end > text && (end[-1] == '\n' || end[-1] == '\r' ||
                          end[-1] == ' ' || end[-1] == '\t')) {
        *--end = '\0';
    }
    return text;
}

static const char *midp_key_label(int key) {
    switch (key) {
    case K_UP:     return "PHONE UP";
    case K_DOWN:   return "PHONE DOWN";
    case K_LEFT:   return "PHONE LEFT";
    case K_RIGHT:  return "PHONE RIGHT";
    case K_SELECT: return "PHONE FIRE";
    case K_SOFT1:  return "SOFT L NOK -6";
    case K_SOFT2:  return "SOFT R NOK -7";
    case K_CLEAR:  return "PHONE CLEAR";
    case K_GAMEA:  return "PHONE GAMEA";
    case K_GAMEB:  return "PHONE GAMEB";
    case K_GAMEC:  return "PHONE GAMEC";
    case K_GAMED:  return "PHONE GAMED";
    case K_SEND:   return "PHONE SEND";
    case K_END:    return "PHONE END";
    case K_GAME_UP:    return "PHONE GAME UP";
    case K_GAME_DOWN:  return "PHONE GAME DOWN";
    case K_GAME_LEFT:  return "PHONE GAME LEFT";
    case K_GAME_RIGHT: return "PHONE GAME RIGHT";
    case -21:      return "SOFT L MOTO -21";
    case -22:      return "SOFT R MOTO -22";
    case 21:       return "SOFT L 21";
    case 22:       return "SOFT R 22";
    case 42:       return "PHONE *";
    case 35:       return "PHONE #";
    case 48:       return "PHONE 0";
    case 49:       return "PHONE 1";
    case 50:       return "PHONE 2";
    case 51:       return "PHONE 3";
    case 52:       return "PHONE 4";
    case 53:       return "PHONE 5";
    case 54:       return "PHONE 6";
    case 55:       return "PHONE 7";
    case 56:       return "PHONE 8";
    case 57:       return "PHONE 9";
    default:       return NULL;
    }
}

static int find_phone_key_index(int code) {
    int i;
    for (i = 0; i < PHONE_KEY_COUNT; i++) {
        if (phone_key_options[i].code == code) return i;
    }
    return -1;
}

static void cycle_bind_value(int bind_index, int direction) {
    int key_index;
    if (bind_index < 0 || bind_index >= BIND_COUNT) return;
    key_index = find_phone_key_index(binds[bind_index]);
    if (key_index < 0) key_index = 0;
    key_index += direction;
    if (key_index < 0) key_index = PHONE_KEY_COUNT - 1;
    if (key_index >= PHONE_KEY_COUNT) key_index = 0;
    binds[bind_index] = phone_key_options[key_index].code;
}

static void apply_key_profile(int profile_index) {
    int i;
    if (profile_index < 0 || profile_index >= KEY_PROFILE_COUNT) return;
    for (i = 0; i < BIND_COUNT; i++) {
        binds[i] = key_profiles[profile_index].defaults[i];
    }
}

static int funkey_bind_index_from_sdl(int32_t code) {
    switch (code) {
    case SDLK_u:
    case SDLK_UP:     return 0;
    case SDLK_d:
    case SDLK_DOWN:   return 1;
    case SDLK_l:
    case SDLK_LEFT:   return 2;
    case SDLK_r:
    case SDLK_RIGHT:  return 3;
    case SDLK_a:
    case SDLK_RETURN:
    case SDLK_SPACE:  return 4;
    case SDLK_b:      return 5;
    case SDLK_x:      return 6;
    case SDLK_y:      return 7;
    case SDLK_m:      return 8;
    case SDLK_n:      return 9;
    case SDLK_s:      return 10;
    case SDLK_k:      return 11;
    default:          return -1;
    }
}

static const char *funkey_short_label(int bind_index) {
    switch (bind_index) {
    case 0:  return "UP";
    case 1:  return "DOWN";
    case 2:  return "LEFT";
    case 3:  return "RIGHT";
    case 4:  return "A";
    case 5:  return "B";
    case 6:  return "X";
    case 7:  return "Y";
    case 8:  return "L1";
    case 9:  return "R1";
    case 10: return "START";
    case 11: return "SELECT";
    default: return "?";
    }
}

static const char *phone_short_label(const char *label) {
    if (strncmp(label, "PHONE ", 6) == 0) return label + 6;
    return label;
}

static void format_phone_bindings(int phone_code, char *out, size_t out_size) {
    int i;
    int first = 1;
    out[0] = '\0';
    for (i = 0; i < BIND_COUNT; i++) {
        if (binds[i] == phone_code) {
            if (!first) {
                strncat(out, ",", out_size - strlen(out) - 1);
            }
            strncat(out, funkey_short_label(i), out_size - strlen(out) - 1);
            first = 0;
        }
    }
    if (first) {
        snprintf(out, out_size, "-");
    }
}

static void shell_quote(const char *src, char *dst, size_t dst_size) {
    size_t used = 0;
    if (dst_size == 0) return;
    dst[used++] = '\'';
    while (*src && used + 5 < dst_size) {
        if (*src == '\'') {
            dst[used++] = '\'';
            dst[used++] = '\\';
            dst[used++] = '\'';
            dst[used++] = '\'';
        } else {
            dst[used++] = *src;
        }
        src++;
    }
    if (used + 1 < dst_size) {
        dst[used++] = '\'';
    }
    dst[used] = '\0';
}

static void score_profile_text(const char *text, int len, int scores[KEY_PROFILE_COUNT]) {
    char lower[2049];
    int i, n;
    if (len <= 0) return;
    n = len;
    if (n > (int)sizeof(lower) - 1) n = (int)sizeof(lower) - 1;
    for (i = 0; i < n; i++) {
        lower[i] = (char)tolower((unsigned char)text[i]);
    }
    lower[n] = '\0';

    if (strstr(lower, "nokia") || strstr(lower, "s40") ||
        strstr(lower, "series40") || strstr(lower, "series 40")) {
        scores[1] += 5;
    }
    if (strstr(lower, "sony") || strstr(lower, "ericsson") ||
        strstr(lower, "sonyericsson") || strstr(lower, "semc")) {
        scores[2] += 5;
    }
    if (strstr(lower, "samsung") || strstr(lower, "sgh-") ||
        strstr(lower, "gt-")) {
        scores[3] += 5;
    }
    if (strstr(lower, "lg-") || strstr(lower, "lge") ||
        strstr(lower, "lg/")) {
        scores[4] += 5;
    }
    if (strstr(lower, "motorola") || strstr(lower, "mot-") ||
        strstr(lower, "v3") || strstr(lower, "razr")) {
        scores[5] += 5;
    }
}

static int suggest_key_profile_for_jar(const char *jar_path) {
    int scores[KEY_PROFILE_COUNT];
    int i;
    int best = 0;
    char quoted[PATH_MAX * 4];
    char cmd[PATH_MAX * 8 + 128];
    FILE *p;
    char buf[2048];

    for (i = 0; i < KEY_PROFILE_COUNT; i++) scores[i] = 0;
    if (!jar_path || !jar_path[0]) return 0;

    score_profile_text(jar_path, strlen(jar_path), scores);
    shell_quote(jar_path, quoted, sizeof(quoted));

    snprintf(cmd, sizeof(cmd),
             "unzip -p %s META-INF/MANIFEST.MF '*.jad' 2>/dev/null; "
             "unzip -l %s 2>/dev/null",
             quoted, quoted);
    p = popen(cmd, "r");
    if (p != NULL) {
        while (fgets(buf, sizeof(buf), p) != NULL) {
            score_profile_text(buf, strlen(buf), scores);
        }
        pclose(p);
    }

    snprintf(cmd, sizeof(cmd), "unzip -p %s 2>/dev/null", quoted);
    p = popen(cmd, "r");
    if (p != NULL) {
        int chunks = 0;
        size_t n;
        while (chunks < 256 && (n = fread(buf, 1, sizeof(buf), p)) > 0) {
            score_profile_text(buf, (int)n, scores);
            chunks++;
        }
        pclose(p);
    }

    for (i = 1; i < KEY_PROFILE_COUNT; i++) {
        if (scores[i] > scores[best]) {
            best = i;
        }
    }
    return best;
}

static const ScalePreset *scale_list_for_source(int source, int *count) {
    if (source >= 0 && source < SOURCE_COUNT) {
        const SourcePreset *preset = &source_presets[source];
        if (preset->width == 128 && preset->height == 128) {
            *count = (int)(sizeof(scale_128x128) / sizeof(scale_128x128[0]));
            return scale_128x128;
        }
        if (preset->width == 128 && preset->height == 160) {
            *count = (int)(sizeof(scale_128x160) / sizeof(scale_128x160[0]));
            return scale_128x160;
        }
        if (preset->width == 176 && preset->height == 208) {
            *count = (int)(sizeof(scale_176x208) / sizeof(scale_176x208[0]));
            return scale_176x208;
        }
        if (preset->width == 176 && preset->height == 220) {
            *count = (int)(sizeof(scale_176x220) / sizeof(scale_176x220[0]));
            return scale_176x220;
        }
        if (preset->width == 208 && preset->height == 208) {
            *count = (int)(sizeof(scale_208x208) / sizeof(scale_208x208[0]));
            return scale_208x208;
        }
        if (preset->width == 240 && preset->height == 320) {
            *count = (int)(sizeof(scale_240x320) / sizeof(scale_240x320[0]));
            return scale_240x320;
        }
        if (preset->width == 320 && preset->height == 240) {
            *count = (int)(sizeof(scale_320x240) / sizeof(scale_320x240[0]));
            return scale_320x240;
        }
    }

    *count = SCALE_COUNT;
    return scale_presets;
}

static const ScalePreset *current_scale_preset(void) {
    int count;
    const ScalePreset *list = scale_list_for_source(source_sel, &count);
    if (scale_sel < 0) scale_sel = 0;
    if (scale_sel >= count) scale_sel = count - 1;
    return &list[scale_sel];
}

static int find_scale_ratio(const char *id) {
    int i;
    if (!id) return 1000;
    for (i = 0; i < SCALE_COUNT; i++) {
        if (strcmp(id, scale_presets[i].id) == 0) return scale_presets[i].ratio;
    }
    #define FIND_SCALE_IN(list) \
        for (i = 0; i < (int)(sizeof(list) / sizeof((list)[0])); i++) { \
            if (strcmp(id, (list)[i].id) == 0) return (list)[i].ratio; \
        }
    FIND_SCALE_IN(scale_128x128)
    FIND_SCALE_IN(scale_128x160)
    FIND_SCALE_IN(scale_176x208)
    FIND_SCALE_IN(scale_176x220)
    FIND_SCALE_IN(scale_208x208)
    FIND_SCALE_IN(scale_240x320)
    FIND_SCALE_IN(scale_320x240)
    #undef FIND_SCALE_IN
    if (strcmp(id, "center-128x128") == 0 || strcmp(id, "crop-128x128") == 0) return 1000;
    if (strcmp(id, "fit-128x160") == 0 || strcmp(id, "fit-176x220") == 0) return 1500;
    if (strcmp(id, "fit-176x208") == 0 || strcmp(id, "fit-208x208") == 0) return 1000;
    if (strcmp(id, "fit-240x320") == 0 || strcmp(id, "fit-320x240") == 0) return 1000;
    return 1000;
}

static int find_scale_for_source(int source, int ratio) {
    int count;
    int i;
    int best = 0;
    int bestDiff;
    const ScalePreset *list = scale_list_for_source(source, &count);
    if (ratio <= 0) ratio = 1000;
    bestDiff = abs(list[0].ratio - ratio);
    for (i = 0; i < count; i++) {
        int diff = abs(list[i].ratio - ratio);
        if (diff == 0) return i;
        if (diff < bestDiff) {
            best = i;
            bestDiff = diff;
        }
    }
    return best;
}

static int find_source_preset(const char *id) {
    int i;
    if (!id) return 0;
    for (i = 0; i < SOURCE_COUNT; i++) {
        if (strcmp(id, source_presets[i].id) == 0) return i;
    }
    return 0;
}

static int find_display_mode(const char *id) {
    int i;
    if (!id) return 5;
    for (i = 0; i < DISPLAY_MODE_COUNT; i++) {
        if (strcmp(id, display_modes[i]) == 0) return i;
    }
    return 5;
}

static int find_key_profile(const char *id) {
    int i;
    if (!id) return 0;
    for (i = 0; i < KEY_PROFILE_COUNT; i++) {
        if (strcmp(id, key_profiles[i].id) == 0) return i;
    }
    return 0;
}

static void load_binds(const char *game_name, const char *jar_path) {
    char path[PATH_MAX];
    FILE *f;
    int i;
    int loaded_scale_ratio = 1000;

    /* start with defaults */
    for (i = 0; i < BIND_COUNT; i++) binds[i] = bind_defaults[i];
    scale_sel = 1;
    source_sel = 0;
    display_mode_sel = 5;
    sound_volume = 100;
    key_profile_sel = 0;

    if (!game_name) return;

    snprintf(path, sizeof(path), BIND_DIR "/%s.cfg", game_name);
    f = fopen(path, "r");
    if (!f) {
        key_profile_sel = suggest_key_profile_for_jar(jar_path);
        apply_key_profile(key_profile_sel);
        save_binds(game_name);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = trim_line(line);
        char *value = trim_line(eq + 1);
        int val = atoi(value);
        if (strcmp(key, "SCALE") == 0) {
            loaded_scale_ratio = find_scale_ratio(value);
            continue;
        }
        if (strcmp(key, "VIEW") == 0 || strcmp(key, "SOURCE") == 0) {
            source_sel = find_source_preset(value);
            continue;
        }
        if (strcmp(key, "DISPLAY_MODE") == 0) {
            display_mode_sel = find_display_mode(value);
            continue;
        }
        if (strcmp(key, "VOLUME") == 0) {
            sound_volume = val;
            if (sound_volume < 0) sound_volume = 0;
            if (sound_volume > 100) sound_volume = 100;
            continue;
        }
        if (strcmp(key, "PROFILE") == 0 || strcmp(key, "KEY_PROFILE") == 0) {
            key_profile_sel = find_key_profile(value);
            apply_key_profile(key_profile_sel);
            continue;
        }
        for (i = 0; i < BIND_COUNT; i++) {
            if (strcmp(key, bind_names[i]) == 0) {
                binds[i] = val;
                break;
            }
        }
    }
    fclose(f);
    scale_sel = find_scale_for_source(source_sel, loaded_scale_ratio);
}

static void save_binds(const char *game_name) {
    char path[PATH_MAX];
    int i;

    if (!game_name) return;

    mkdir(BIND_DIR, 0777);
    snprintf(path, sizeof(path), BIND_DIR "/%s.cfg", game_name);
    FILE *f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "SCALE=%s\n", current_scale_preset()->id);
    fprintf(f, "VIEW=%s\n", source_presets[source_sel].id);
    fprintf(f, "DISPLAY_MODE=%s\n", display_modes[display_mode_sel]);
    fprintf(f, "VOLUME=%d\n", sound_volume);
    fprintf(f, "PROFILE=%s\n", key_profiles[key_profile_sel].id);

    for (i = 0; i < BIND_COUNT; i++)
        fprintf(f, "%s=%d\n", bind_names[i], binds[i]);

    fclose(f);
}

/* ─────────────────────────────────────────────────────
 *  draw helpers
 * ──────────────────────────────────────────────────── */
static void draw_header(const char *title, int y) {
    stringColor(screen, 4, y, title, 0x7df9ffff);
}

static int draw_item(int x, int y, const char *text, int selected) {
    if (selected) {
        boxColor(screen, 0, y - 1, SCR_W - 1, y + 9, 0x003366ff);
        stringColor(screen, x, y, text, 0x00ff88ff);
    } else {
        stringColor(screen, x, y, text, 0xffffffff);
    }
    return y + 11;
}

#define VISIBLE_ITEMS  17

/* ─────────────────────────────────────────────────────
 *  browser screen
 * ──────────────────────────────────────────────────── */
static void draw_browser(void) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x10, 0x18, 0x20));

    int y = 6;
    draw_header("phoneME Launcher", y);
    y += 12;

    if (game_count == 0) {
        stringColor(screen, 4, y, java_dir, 0xff6060ff);
        stringColor(screen, 4, y + 2, "is empty", 0xff6060ff);
        stringColor(screen, 4, y + 12, "Add .jar files to play", 0xffffffff);
        SDL_Flip(screen);
        return;
    }

    /* scroll window */
    if (game_sel < game_scroll) game_scroll = game_sel;
    if (game_sel >= game_scroll + VISIBLE_ITEMS) game_scroll = game_sel - VISIBLE_ITEMS + 1;

    int i;
    for (i = game_scroll; i < game_count && y < SCR_H - 16; i++) {
        const char *name = game_stem(games[i]);
        y = draw_item(6, y, name, i == game_sel);
    }

    /* help bar */
    y = SCR_H - 10;
    stringColor(screen, 2, y, "START:launch",  0x607078ff);
    stringColor(screen, 74, y, "A:keys",        0x607078ff);
    stringColor(screen, 130, y, "B:refresh",    0x607078ff);
    stringColor(screen, 192, y, "X:exit",       0x607078ff);

    SDL_Flip(screen);
}

/* ─────────────────────────────────────────────────────
 *  keybind settings screen
 * ──────────────────────────────────────────────────── */
static void draw_keybinds(void) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x10, 0x18, 0x20));

    int y = 6;
    char buf[128];
    snprintf(buf, sizeof(buf), "Controls: %s", game_stem(games[game_sel]));
    draw_header(buf, y);
    y += 12;

    if (bind_sel < bind_scroll) bind_scroll = bind_sel;
    if (bind_sel >= bind_scroll + VISIBLE_ITEMS) bind_scroll = bind_sel - VISIBLE_ITEMS + 1;

    int i;
    for (i = bind_scroll; i < PHONE_KEY_COUNT + BIND_ROW_OFFSET && y < SCR_H - 24; i++) {
        if (i == 0) {
            snprintf(buf, sizeof(buf), "SCALE  %s", current_scale_preset()->label);
        } else if (i == 1) {
            snprintf(buf, sizeof(buf), "VIEW   %s", source_presets[source_sel].label);
        } else if (i == 2) {
            snprintf(buf, sizeof(buf), "MODE   %s", display_modes[display_mode_sel]);
        } else if (i == 3) {
            snprintf(buf, sizeof(buf), "PROFILE %s", key_profiles[key_profile_sel].label);
        } else {
            int phone_index = i - BIND_ROW_OFFSET;
            char assigned[64];
            format_phone_bindings(phone_key_options[phone_index].code,
                                  assigned, sizeof(assigned));
            snprintf(buf, sizeof(buf), "%-10s %s",
                     phone_short_label(phone_key_options[phone_index].label),
                     assigned);
        }
        y = draw_item(4, y, buf, i == bind_sel);
    }

    /* capture prompt or help bar */
    y = SCR_H - 10;
    if (capture_mode) {
        stringColor(screen, 4, y - 14, "Press FunKey button to bind", 0x00ff88ff);
        stringColor(screen, 4, y, "POWER:cancel", 0x607078ff);
    } else {
        stringColor(screen, 4, y, "A:bind/change  B:save", 0x607078ff);
    }

    SDL_Flip(screen);
}

/* ─────────────────────────────────────────────────────
 *  launching overlay
 * ──────────────────────────────────────────────────── */
static void draw_launching(void) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x10, 0x18, 0x20));

    char buf[256];
    snprintf(buf, sizeof(buf), "Launching %s...", game_stem(games[game_sel]));
    stringColor(screen, 4, SCR_H / 2 - 5, buf, 0x00ff88ff);

    SDL_Flip(screen);
    SDL_Delay(200);
}

static void draw_launch_error(const char *line1, const char *line2) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x10, 0x18, 0x20));
    stringColor(screen, 4, SCR_H / 2 - 16, line1, 0xff6060ff);
    stringColor(screen, 4, SCR_H / 2 - 4, line2, 0xffffffff);
    stringColor(screen, 4, SCR_H / 2 + 12, "Press B to return", 0x607078ff);
    SDL_Flip(screen);
    SDL_Delay(2500);
}

/* ─────────────────────────────────────────────────────
 *  launch helpers – copy runtime files
 * ──────────────────────────────────────────────────── */
static int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }
    char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
    chmod(dst, 0755);
    return 0;
}

static int same_file_contents(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb");
    FILE *fb = fopen(b, "rb");
    int same = 1;
    if (!fa || !fb) {
        if (fa) fclose(fa);
        if (fb) fclose(fb);
        return 0;
    }
    for (;;) {
        unsigned char ba[4096], bb[4096];
        size_t na = fread(ba, 1, sizeof(ba), fa);
        size_t nb = fread(bb, 1, sizeof(bb), fb);
        if (na != nb || memcmp(ba, bb, na) != 0) {
            same = 0;
            break;
        }
        if (na == 0) break;
    }
    fclose(fa);
    fclose(fb);
    return same;
}

static void remove_tree(const char *path) {
    char cmd[PATH_MAX + 32];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", path);
    system(cmd);
}

static int jar_has_entry(const char *jar_path, const char *entry) {
    char cmd[PATH_MAX * 2 + 256];
    int rc;
    snprintf(cmd, sizeof(cmd),
        "unzip -l \"%s\" \"%s\" >/dev/null 2>&1",
        jar_path, entry);
    rc = system(cmd);
    return rc == 0;
}

static void copy_dir(const char *src_base, const char *dst_base) {
    DIR *d = opendir(src_base);
    if (!d) return;
    mkdir(dst_base, 0777);
    struct dirent *ent;
    char src_path[PATH_MAX], dst_path[PATH_MAX];
    while ((ent = readdir(d)) != NULL) {
        struct stat st;
        if (ent->d_name[0] == '.') continue;
        snprintf(src_path, sizeof(src_path), "%s/%s", src_base, ent->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_base, ent->d_name);
        if (stat(src_path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            copy_dir(src_path, dst_path);
        } else if (S_ISREG(st.st_mode)) {
            copy_file(src_path, dst_path);
        }
    }
    closedir(d);
}

/* ─────────────────────────────────────────────────────
 *  launch
 * ──────────────────────────────────────────────────── */
static void launch_game(int idx) {
    const char *jar_path = games[idx];

    draw_launching();

    char self_dir[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", self_dir, sizeof(self_dir) - 1);
    if (len >= 0) {
        self_dir[len] = '\0';
        char *s = strrchr(self_dir, '/');
        if (s) *s = '\0';
    } else {
        strcpy(self_dir, ".");
    }

    mkdir(PM_DIR, 0777);
    mkdir(BIN_DIR, 0777);
    mkdir(PM_DIR "/lib", 0777);
    mkdir(PM_DIR "/appdb", 0777);

    /* copy phoneME runtime to persistent dir only when OPK runtime changed */
    {
        char src[PATH_MAX], version_src[PATH_MAX], version_dst[PATH_MAX];
        int runtime_current;
        snprintf(version_src, sizeof(version_src), "%s/runtime-version", self_dir);
        snprintf(version_dst, sizeof(version_dst), "%s/runtime-version", PM_DIR);
        runtime_current = same_file_contents(version_src, version_dst);
        if (!runtime_current) {
            remove_tree(BIN_DIR);
            remove_tree(PM_DIR "/lib");
            mkdir(BIN_DIR, 0777);
            mkdir(PM_DIR "/lib", 0777);
            snprintf(src, sizeof(src), "%s/bin", self_dir);
            copy_dir(src, BIN_DIR);
            snprintf(src, sizeof(src), "%s/lib", self_dir);
            copy_dir(src, PM_DIR "/lib");
            copy_file(version_src, version_dst);
        }
        /* appdb: seed on first launch only, preserve RMS state */
        {
            DIR *check = opendir(PM_DIR "/appdb");
            if (check != NULL) {
                closedir(check);
            } else {
                snprintf(src, sizeof(src), "%s/appdb", self_dir);
                copy_dir(src, PM_DIR "/appdb");
            }
        }

        char cmd[PATH_MAX * 2 + 64];
        snprintf(cmd, sizeof(cmd), "chmod +x %s/* 2>/dev/null || true", BIN_DIR);
        if (!runtime_current) system(cmd);
    }

    /* extract MIDlet main class from JAR manifest */
    char main_class[256] = {0};
    char jad_path[PATH_MAX];
    {
        int jar_len = strlen(jar_path);
        snprintf(jad_path, sizeof(jad_path), "%.*s.jad", jar_len - 4, jar_path);

        char extract_cmd[PATH_MAX * 2 + 256];
        snprintf(extract_cmd, sizeof(extract_cmd),
            "unzip -p \"%s\" META-INF/MANIFEST.MF 2>/dev/null | "
            "grep -i '^MIDlet-1:' | sed 's/.*://' | tr -d '\\r' | "
            "awk -F, '{print $3}' | tr -d ' ' > "
            "%s/main-class", jar_path, PM_DIR);
        fflush(stdout);
        system(extract_cmd);

        /* create .jad from manifest so phoneME sees app properties */
        snprintf(extract_cmd, sizeof(extract_cmd),
            "unzip -p \"%s\" META-INF/MANIFEST.MF 2>/dev/null > \"%s\"",
            jar_path, jad_path);
        system(extract_cmd);
    }

    {
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/main-class", PM_DIR);
        FILE *mf = fopen(path, "r");
        if (mf) {
            if (fgets(main_class, sizeof(main_class), mf)) {
                char *nl = strchr(main_class, '\n');
                if (nl) *nl = '\0';
            }
            fclose(mf);
        }
    }

    if (main_class[0] == '\0') {
        FILE *log = fopen(PM_DIR "/runmidlet.log", "w");
        if (log != NULL) {
            fprintf(log, "Could not infer MIDlet main class from %s\n", jar_path);
            fprintf(log, "The JAR is missing MIDlet-1 in META-INF/MANIFEST.MF.\n");
            fprintf(log, "This usually means it is not a MIDP MIDlet, or it needs a .jad file.\n");
            fclose(log);
        }
        draw_launch_error("No MIDlet-1 found", "Not a runnable MIDP jar");
        return;
    }

    /* set env vars BEFORE fork so child inherits them + system PATH etc */
    setenv("MIDP_HOME", PM_DIR, 1);
    setenv("PHONEME_HEAP_MB", "16", 1);
    setenv("PHONEME_ENABLE_AUDIO", "1", 1);
    setenv("PHONEME_TIMIDITY_SYNTHETIC", "1", 1);
    setenv("PHONEME_ENABLE_GP2X_KEYS", "1", 1);
    setenv("PHONEME_KEY_PROFILE", key_profiles[key_profile_sel].id, 1);
    setenv("PHONEME_SCALE_PRESET", current_scale_preset()->id, 1);
    setenv("PHONEME_SCALE_MODE", display_modes[display_mode_sel], 1);
    {
        char cfg_path[PATH_MAX];
        snprintf(cfg_path, sizeof(cfg_path), BIND_DIR "/%s.cfg", game_stem(jar_path));
        setenv("PHONEME_CONFIG_PATH", cfg_path, 1);
    }
    {
        char volume[16];
        snprintf(volume, sizeof(volume), "%d", sound_volume);
        setenv("PHONEME_SOUND_VOLUME", volume, 1);
    }
    {
        char ratio[16];
        snprintf(ratio, sizeof(ratio), "%d", current_scale_preset()->ratio);
        setenv("PHONEME_SCALE_RATIO", ratio, 1);
    }
    if (strcmp(source_presets[source_sel].id, "full") == 0) {
        setenv("PHONEME_SOURCE_SIZE", "full", 1);
        unsetenv("PHONEME_LCD_SIZE");
    } else if (source_presets[source_sel].width > 0 && source_presets[source_sel].height > 0) {
        char size[32];
        snprintf(size, sizeof(size), "%dx%d",
                 source_presets[source_sel].width,
                 source_presets[source_sel].height);
        setenv("PHONEME_SOURCE_SIZE", size, 1);
        setenv("PHONEME_LCD_SIZE", size, 1);
    } else {
        unsetenv("PHONEME_SOURCE_SIZE");
        unsetenv("PHONEME_LCD_SIZE");
    }

    int i;
    for (i = 0; i < BIND_COUNT; i++) {
        char kvenv[128], val[32];
        snprintf(kvenv, sizeof(kvenv), "J2ME_KEY_%s", bind_names[i]);
        snprintf(val, sizeof(val), "%d", binds[i]);
        setenv(kvenv, val, 1);
        setenv(bind_gp2x_env[i], val, 1);
    }

    char runmidlet_path[PATH_MAX];
    snprintf(runmidlet_path, sizeof(runmidlet_path), "%s/runMidlet", BIN_DIR);

    /* release SDL (framebuffer, input) so phoneME can take it */
    SDL_Quit();

    pid_t pid = fork();
    if (pid == 0) {
        /* child — redirect stdout+stderr to runmidlet log */
        int log_fd = open(PM_DIR "/runmidlet.log",
                          O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (log_fd >= 0) {
            dup2(log_fd, STDOUT_FILENO);
            dup2(log_fd, STDERR_FILENO);
            if (log_fd > 2) close(log_fd);
        }
        if (getenv("PM_LAUNCH_TRACE") != NULL) {
            printf("pm-launch trace\n");
            printf("pm-launch jar=%s\n", jar_path);
            printf("pm-launch main=%s\n", main_class);
            printf("pm-launch PHONEME_ENABLE_AUDIO=%s\n",
                   getenv("PHONEME_ENABLE_AUDIO") ? getenv("PHONEME_ENABLE_AUDIO") : "");
            printf("pm-launch PHONEME_HEAP_MB=%s\n",
                   getenv("PHONEME_HEAP_MB") ? getenv("PHONEME_HEAP_MB") : "");
            printf("pm-launch scale preset=%s ratio=%s lcd=%s mode=%s\n",
                   getenv("PHONEME_SCALE_PRESET") ? getenv("PHONEME_SCALE_PRESET") : "",
                   getenv("PHONEME_SCALE_RATIO") ? getenv("PHONEME_SCALE_RATIO") : "",
                   getenv("PHONEME_LCD_SIZE") ? getenv("PHONEME_LCD_SIZE") : "",
                   getenv("PHONEME_SCALE_MODE") ? getenv("PHONEME_SCALE_MODE") : "");
            printf("pm-launch view preset=%s source=%s\n",
                   source_presets[source_sel].id,
                   getenv("PHONEME_SOURCE_SIZE") ? getenv("PHONEME_SOURCE_SIZE") : "");
            printf("pm-launch sound volume=%s config=%s\n",
                   getenv("PHONEME_SOUND_VOLUME") ? getenv("PHONEME_SOUND_VOLUME") : "",
                   getenv("PHONEME_CONFIG_PATH") ? getenv("PHONEME_CONFIG_PATH") : "");
            printf("pm-launch key profile=%s\n", key_profiles[key_profile_sel].id);
            for (i = 0; i < BIND_COUNT; i++) {
                printf("pm-launch key %s %s=%s\n",
                       bind_names[i], bind_gp2x_env[i],
                       getenv(bind_gp2x_env[i]) ? getenv(bind_gp2x_env[i]) : "");
            }
            fflush(stdout);
        }

        /* close other inherited FDs */
        int fd;
        for (fd = 3; fd < 256; fd++) close(fd);

        chdir(BIN_DIR);
        {
            char classpath[PATH_MAX + 1];
            snprintf(classpath, sizeof(classpath), "%s", jar_path);
        char *argv[] = {
            "runMidlet",
            "-classpathext",
            classpath,
            "internal",
            main_class,
            NULL
        };
        execv(runmidlet_path, argv);
        }
        exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        sync();

        /* re-init SDL for browser */
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
        screen = SDL_SetVideoMode(SCR_W, SCR_H, 16, SDL_SWSURFACE);
        SDL_ShowCursor(SDL_DISABLE);
    }
}

/* ─────────────────────────────────────────────────────
 *  input handlers
 * ──────────────────────────────────────────────────── */
static void handle_key_browser(SDL_KeyboardEvent *kev) {
    if (kev->type != SDL_KEYDOWN) return;

    switch (kev->keysym.sym) {
    case SDLK_UP:    case SDLK_u:
        game_sel--;
        if (game_sel < 0) game_sel = game_count - 1;
        break;
    case SDLK_DOWN:  case SDLK_d:
        game_sel++;
        if (game_sel >= game_count) game_sel = 0;
        break;
    case SDLK_LEFT:  case SDLK_l:
        game_sel -= 5;
        if (game_sel < 0) game_sel = 0;
        break;
    case SDLK_RIGHT: case SDLK_r:
        game_sel += 5;
        if (game_sel >= game_count) game_sel = game_count - 1;
        break;
    case SDLK_RETURN: case SDLK_SPACE: case SDLK_a: /* A = SELECT */
        if (game_count > 0) {
            load_binds(game_stem(games[game_sel]), games[game_sel]);
            bind_sel = 0;
            bind_scroll = 0;
            state = STATE_KEYBINDS;
        }
        break;
    case SDLK_s: /* START = SOFT2 */
        if (game_count > 0) {
            load_binds(game_stem(games[game_sel]), games[game_sel]);
            launch_game(game_sel);
            state = STATE_BROWSER;
        }
        break;
    case SDLK_b: /* B = refresh */
        scan_games();
        break;
    case SDLK_x: /* X = exit */
    case SDLK_q: /* POWER/FN = SOFT1 */
    case SDLK_ESCAPE:
        exit(0);
    default:
        break;
    }
}

static void handle_key_keybinds(SDL_KeyboardEvent *kev) {
    if (kev->type != SDL_KEYDOWN) return;

    if (capture_mode) {
        int32_t code = (int32_t)kev->keysym.sym;
        int bind_index;
        int phone_index;

        if (code == SDLK_ESCAPE || code == SDLK_q) {
            capture_mode = 0;
            return;
        }

        bind_index = funkey_bind_index_from_sdl(code);
        phone_index = bind_sel - BIND_ROW_OFFSET;
        if (bind_index >= 0 && phone_index >= 0 && phone_index < PHONE_KEY_COUNT) {
            binds[bind_index] = phone_key_options[phone_index].code;
        }
        capture_mode = 0;
        return;
    }

    switch (kev->keysym.sym) {
    case SDLK_UP:    case SDLK_u:
        bind_sel--;
        if (bind_sel < 0) bind_sel = PHONE_KEY_COUNT + BIND_ROW_OFFSET - 1;
        break;
    case SDLK_DOWN:  case SDLK_d:
        bind_sel++;
        if (bind_sel >= PHONE_KEY_COUNT + BIND_ROW_OFFSET) bind_sel = 0;
        break;

    case SDLK_LEFT:  case SDLK_l:
        if (bind_sel == 0) {
            int scale_count;
            scale_list_for_source(source_sel, &scale_count);
            scale_sel--;
            if (scale_sel < 0) scale_sel = scale_count - 1;
        } else if (bind_sel == 1) {
            int ratio = current_scale_preset()->ratio;
            source_sel--;
            if (source_sel < 0) source_sel = SOURCE_COUNT - 1;
            scale_sel = find_scale_for_source(source_sel, ratio);
        } else if (bind_sel == 2) {
            display_mode_sel--;
            if (display_mode_sel < 0) display_mode_sel = DISPLAY_MODE_COUNT - 1;
        } else if (bind_sel == 3) {
            key_profile_sel--;
            if (key_profile_sel < 0) key_profile_sel = KEY_PROFILE_COUNT - 1;
            apply_key_profile(key_profile_sel);
        }
        break;
    case SDLK_RIGHT: case SDLK_r:
        if (bind_sel == 0) {
            int scale_count;
            scale_list_for_source(source_sel, &scale_count);
            scale_sel++;
            if (scale_sel >= scale_count) scale_sel = 0;
        } else if (bind_sel == 1) {
            int ratio = current_scale_preset()->ratio;
            source_sel++;
            if (source_sel >= SOURCE_COUNT) source_sel = 0;
            scale_sel = find_scale_for_source(source_sel, ratio);
        } else if (bind_sel == 2) {
            display_mode_sel++;
            if (display_mode_sel >= DISPLAY_MODE_COUNT) display_mode_sel = 0;
        } else if (bind_sel == 3) {
            key_profile_sel++;
            if (key_profile_sel >= KEY_PROFILE_COUNT) key_profile_sel = 0;
            apply_key_profile(key_profile_sel);
        }
        break;

    case SDLK_RETURN: case SDLK_SPACE: case SDLK_a: /* A = next value */
        if (bind_sel == 0) {
            int scale_count;
            scale_list_for_source(source_sel, &scale_count);
            scale_sel++;
            if (scale_sel >= scale_count) scale_sel = 0;
        } else if (bind_sel == 1) {
            int ratio = current_scale_preset()->ratio;
            source_sel++;
            if (source_sel >= SOURCE_COUNT) source_sel = 0;
            scale_sel = find_scale_for_source(source_sel, ratio);
        } else if (bind_sel == 2) {
            display_mode_sel++;
            if (display_mode_sel >= DISPLAY_MODE_COUNT) display_mode_sel = 0;
        } else if (bind_sel == 3) {
            key_profile_sel++;
            if (key_profile_sel >= KEY_PROFILE_COUNT) key_profile_sel = 0;
            apply_key_profile(key_profile_sel);
        } else {
            capture_mode = 1;
        }
        break;

    case SDLK_b: /* back / cancel capture */
        if (capture_mode) {
            capture_mode = 0;
        } else {
            save_binds(game_stem(games[game_sel]));
            state = STATE_BROWSER;
        }
        break;

    case SDLK_ESCAPE: case SDLK_q:
        save_binds(game_stem(games[game_sel]));
        state = STATE_BROWSER;
        break;

    default:
        break;
    }
}

/* ─────────────────────────────────────────────────────
 *  main
 * ──────────────────────────────────────────────────── */
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    /* init java_dir from env */
    {
        const char *env_java = getenv("PHONEME_JAVA_DIR");
        if (env_java && env_java[0]) {
            snprintf(java_dir, sizeof(java_dir), "%s", env_java);
        }
    }

    /* init SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(SCR_W, SCR_H, 16, SDL_SWSURFACE);
    if (!screen) {
        fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_ShowCursor(SDL_DISABLE);
    SDL_WM_SetCaption("phoneME Launcher", NULL);

    /* scan games */
    scan_games();

    /* event loop */
    int running = 1;
    SDL_Event ev;

    while (running) {
        switch (state) {
        case STATE_BROWSER:   draw_browser();   break;
        case STATE_KEYBINDS:  draw_keybinds();  break;
        case STATE_LAUNCHING: draw_launching(); break;
        }

        while (SDL_WaitEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running = 0; break; }
            if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
                switch (state) {
                case STATE_BROWSER:
                    handle_key_browser(&ev.key);
                    break;
                case STATE_KEYBINDS:
                    handle_key_keybinds(&ev.key);
                    break;
                case STATE_LAUNCHING:
                    break;
                }
                if (ev.type == SDL_KEYDOWN) break; /* one event per frame */
            }
        }
    }

    return 0;
}
