#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <SDL_gfxPrimitives_font.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "pm-unicode-font.h"

/* ── screen ─────────────────────────────────────────── */
#define SCR_W  240
#define SCR_H  240

/* ── directories ────────────────────────────────────── */
#ifndef PM_DIR
#define PM_DIR "/mnt/FunKey/.pm"
#endif
#ifndef BROWSE_DIR
#define BROWSE_DIR "/mnt/java"
#endif
#define BIND_DIR   PM_DIR "/keybinds"
#define BIN_DIR    PM_DIR "/bin"
#define RMS_DIR    PM_DIR "/rms"
#define FAVORITES_FILE PM_DIR "/favorites.txt"
#define DEBUG_FILE     PM_DIR "/debug.cfg"
#define RESTART_FLAG   PM_DIR "/restart-request"
#ifdef RUNTIME_LOG
#define LOG_FILE       RUNTIME_LOG
#else
#define LOG_FILE       PM_DIR "/runmidlet.log"
#endif
#define LOG_PREV_FILE  PM_DIR "/runmidlet.prev.log"

static char java_dir[PATH_MAX] = BROWSE_DIR;

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
#define SETTINGS_ROW_COUNT 7
#define SETTINGS_ROW_ZOOM 0
#define SETTINGS_ROW_SIZE 1
#define SETTINGS_ROW_SCREEN 2
#define SETTINGS_ROW_FPS 3
#define SETTINGS_ROW_PROFILE 4
#define SETTINGS_ROW_KEYS 5
#define SETTINGS_ROW_DELETE 6
#define KEY_EDIT_PROFILE_ROW 0
#define KEY_EDIT_ROW_OFFSET 1

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
    { "doja", "DOJA/I-APPLI",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, K_SOFT1, K_CLEAR, K_SOFT2,
        42, 35, K_SOFT2, K_SOFT1 } },
    { "nokia-s40", "NOKIA/S40",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, 53, K_GAMEC, K_GAMED,
        42, 35, K_SOFT2, K_SOFT1 } },
    { "sony-ericsson", "SONY ERICSSON",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, 53, K_GAMEC, K_GAMED,
        K_CLEAR, 35, K_SOFT1, K_SOFT2 } },
    { "samsung", "SAMSUNG",
      { 50, 56, 52, 54, 53, K_SELECT, K_GAMEC, K_GAMED,
        42, 35, K_SOFT2, K_SOFT1 } },
    { "lg", "LG",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, 53, K_CLEAR, K_GAMED,
        42, 35, K_SOFT1, K_SOFT2 } },
    { "motorola", "MOTOROLA",
      { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SELECT, K_GAMEA, K_GAMEC, K_GAMED,
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

typedef struct {
    const char *id;
    const char *label;
} DisplayModePreset;

typedef struct {
    const char *id;
    const char *label;
} FpsPreset;

static const ScalePreset scale_presets[] = {
    { "zoom-075", "75%", "zoom", 750 },
    { "zoom-100", "100%", "zoom", 1000 },
    { "zoom-120", "120%", "zoom", 1200 },
    { "zoom-140", "140%", "zoom", 1400 },
    { "zoom-150", "150%", "zoom", 1500 },
    { "zoom-160", "160%", "zoom", 1600 },
    { "zoom-170", "170%", "zoom", 1700 },
    { "zoom-180", "180%", "zoom", 1800 },
    { "zoom-185", "185%", "zoom", 1850 }
};

#define SCALE_COUNT  ((int)(sizeof(scale_presets) / sizeof(scale_presets[0])))

static const ScalePreset scale_128x128[] = {
    { "zoom-150", "150%", "zoom", 1500 },
    { "zoom-175", "175%", "zoom", 1750 },
    { "zoom-1875", "188%", "zoom", 1875 },
    { "zoom-200", "200%", "zoom", 2000 }
};

static const ScalePreset scale_128x160[] = {
    { "zoom-140", "140%", "zoom", 1400 },
    { "zoom-150", "150%", "zoom", 1500 },
    { "zoom-160", "160%", "zoom", 1600 },
    { "zoom-170", "170%", "zoom", 1700 }
};

static const ScalePreset scale_176x208[] = {
    { "zoom-100", "100%", "zoom", 1000 },
    { "zoom-110", "110%", "zoom", 1100 },
    { "zoom-115", "115%", "zoom", 1150 },
    { "zoom-120", "120%", "zoom", 1200 }
};

static const ScalePreset scale_176x220[] = {
    { "zoom-100", "100%", "zoom", 1000 },
    { "zoom-105", "105%", "zoom", 1050 },
    { "zoom-109", "109%", "zoom", 1090 },
    { "zoom-115", "115%", "zoom", 1150 },
    { "zoom-120", "120%", "zoom", 1200 }
};

static const ScalePreset scale_208x208[] = {
    { "zoom-100", "100%", "zoom", 1000 },
    { "zoom-110", "110%", "zoom", 1100 },
    { "zoom-115", "115%", "zoom", 1150 }
};

static const ScalePreset scale_240x320[] = {
    { "zoom-070", "Safe 70%", "zoom", 700 },
    { "zoom-075", "Fit all 75%", "zoom", 750 },
    { "zoom-080", "Crop 80%", "zoom", 800 },
    { "zoom-085", "Crop 85%", "zoom", 850 },
    { "zoom-090", "Crop 90%", "zoom", 900 },
    { "zoom-100", "Full width", "zoom", 1000 }
};

static const ScalePreset scale_320x240[] = {
    { "zoom-070", "Safe 70%", "zoom", 700 },
    { "zoom-075", "Fit all 75%", "zoom", 750 },
    { "zoom-080", "Crop 80%", "zoom", 800 },
    { "zoom-085", "Crop 85%", "zoom", 850 },
    { "zoom-090", "Crop 90%", "zoom", 900 },
    { "zoom-100", "Full height", "zoom", 1000 }
};

static const SourcePreset source_presets[] = {
    { "auto", "Auto", 0, 0 },
    { "full", "Full buffer", 0, 0 },
    { "128x128", "128x128", 128, 128 },
    { "128x160", "128x160", 128, 160 },
    { "176x208", "176x208", 176, 208 },
    { "176x220", "176x220", 176, 220 },
    { "208x208", "208x208", 208, 208 },
    { "240x320", "Tall 240x320", 240, 320 },
    { "320x240", "Wide 320x240", 320, 240 }
};

#define SOURCE_COUNT  ((int)(sizeof(source_presets) / sizeof(source_presets[0])))

static const DisplayModePreset display_modes[] = {
    { "auto", "Auto" },
    { "fit", "Fit all" },
    { "fill", "Stretch" },
    { "crop", "Crop fill" },
    { "center", "Center" },
    { "zoom", "Manual zoom" },
    { "integer", "Integer" }
};

#define DISPLAY_MODE_COUNT ((int)(sizeof(display_modes) / sizeof(display_modes[0])))

static const FpsPreset fps_limits[] = {
    { "off", "Off" },
    { "20", "20 FPS" },
    { "25", "25 FPS" },
    { "30", "30 FPS" },
    { "50", "50 FPS" },
    { "60", "60 FPS" }
};

#define FPS_LIMIT_COUNT ((int)(sizeof(fps_limits) / sizeof(fps_limits[0])))

/* ── game list ──────────────────────────────────────── */
/* the list grows as needed; names and paths are capped per entry */
#define GAME_STR_MAX 1024
typedef struct {
    char *path;
    char label[GAME_STR_MAX];
    char config[GAME_STR_MAX];
    char title[128];
    char jam_path[GAME_STR_MAX];
    char sp_path[GAME_STR_MAX];
    char app_class[256];
    char app_param[256];
    char app_ver[128];
    int is_dir;
    int is_doja;
    int is_favorites_folder;
    int is_favorite;
} GameEntry;

/*
 * Platforms the runtime can start. A game is DoJa (i-mode i-appli) when
 * a .jam descriptor with an AppClass belongs to its jar, MIDP otherwise.
 */
enum { PLATFORM_MIDP = 0, PLATFORM_DOJA = 1 };

typedef struct {
    const char *id;
    const char *main_class;   /* NULL: MIDlet-1 of the manifest */
    const char *support_jar;  /* appended to the classpath, relative to PM_DIR */
    const char *heap_mb;
    const char *key_profile;  /* default key profile, NULL: guess from the jar */
    const char *encoding;     /* microedition.encoding, NULL: default */
    const char *tag;          /* shown before the name in the list */
} Platform;

static const Platform platforms[] = {
    { "midp", NULL, NULL, "16", NULL, NULL, "" },
    { "doja", "com.funkey.doja.DoJaMIDlet", "lib/doja-support.jar", "32", "doja", "SJIS", "i " }
};

static GameEntry *games = NULL;
static int    games_cap  = 0;
static int    game_count = 0;
static int    game_sel   = 0;        /* selected index in browser */
static int    game_scroll = 0;       /* first visible line */

/* room for one more entry; 0 when out of memory */
static int grow_games(void) {
    if (game_count < games_cap) return 1;
    int cap = games_cap ? games_cap * 2 : 64;
    GameEntry *g = (GameEntry *)realloc(games, (size_t)cap * sizeof(GameEntry));
    if (g == NULL) return 0;
    games = g;
    games_cap = cap;
    return 1;
}
static char   current_rel_dir[PATH_MAX] = "";
static int    browsing_favorites = 0;

#define NAV_STACK_MAX 64
typedef struct {
    char path[PATH_MAX];
    int scroll;
    int is_favorites_folder;
} BrowserReturn;

static BrowserReturn nav_stack[NAV_STACK_MAX];
static int nav_depth = 0;

/* ── current key bindings (loaded / defaults) ───────── */
static int    binds[BIND_COUNT];
static int    scale_sel = 0;
static int    source_sel = 0;
static int    display_mode_sel = 5;
static int    fps_limit_sel = 3;
static int    sound_volume = 100;
static char   synth_id[16] = "hybrid";   /* MIDI synth preset, set in the in-game menu */
static char   filter_id[16] = "sharp";   /* scaling filter, set in the in-game menu */
static int    pan_x, pan_y;              /* view position for cropped modes, -1000..1000 */
static int    key_profile_sel = 0;
static int    bind_sel   = 0;        /* selected row in settings */
static int    bind_scroll = 0;
static int    capture_mode = 0;       /* 1 = waiting for key press to bind */
static int    key_edit_mode = 0;      /* 0 = settings menu, 1 = key rows */
static int    delete_confirm = 0;

/* ── state ──────────────────────────────────────────── */
enum { STATE_BROWSER, STATE_KEYBINDS, STATE_LAUNCHING, STATE_RESOLUTION,
       STATE_DEBUG, STATE_LOGVIEW, STATE_CRASH };
static int state = STATE_BROWSER;
static int launching_game_idx = -1;

/* ── SDL objects ────────────────────────────────────── */
static SDL_Surface *screen = NULL;

#define PM_FONT_ADVANCE 8
#define PM_ASCII_ADVANCE 8
#define PM_FONT_LINE_H  11
#define PM_COLOR_FAVORITE 0xffcc33ff

static const unsigned char *pm_select_font_bitmap(uint32_t cp) {
    int i;
    int count = (int)(uintptr_t)PmFontBitmaps[0];
    unsigned char hi = (unsigned char)((cp >> 8) & 0xff);
    unsigned char lo = (unsigned char)(cp & 0xff);

    for (i = 1; i <= count; i++) {
        const unsigned char *font = PmFontBitmaps[i];
        if (hi == font[PM_FONT_CODE_RANGE_HIGH] &&
            lo >= font[PM_FONT_CODE_FIRST_LOW] &&
            lo <= font[PM_FONT_CODE_LAST_LOW]) {
            return font;
        }
    }
    return NULL;
}

static uint32_t pm_next_utf8(const char **text) {
    const unsigned char *s = (const unsigned char *)*text;
    uint32_t cp;
    static const uint16_t cp1251_extra[64] = {
        0x0402,0x0403,0x201a,0x0453,0x201e,0x2026,0x2020,0x2021,
        0x20ac,0x2030,0x0409,0x2039,0x040a,0x040c,0x040b,0x040f,
        0x0452,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,
        0x0000,0x2122,0x0459,0x203a,0x045a,0x045c,0x045b,0x045f,
        0x00a0,0x040e,0x045e,0x0408,0x00a4,0x0490,0x00a6,0x00a7,
        0x0401,0x00a9,0x0404,0x00ab,0x00ac,0x00ad,0x00ae,0x0407,
        0x00b0,0x00b1,0x0406,0x0456,0x0491,0x00b5,0x00b6,0x00b7,
        0x0451,0x2116,0x0454,0x00bb,0x0458,0x0405,0x0455,0x0457
    };

    if (s[0] < 0x80) {
        *text += 1;
        return s[0];
    }
    if ((s[0] & 0xe0) == 0xc0 && (s[1] & 0xc0) == 0x80) {
        cp = ((uint32_t)(s[0] & 0x1f) << 6) | (uint32_t)(s[1] & 0x3f);
        *text += 2;
        return cp;
    }
    if ((s[0] & 0xf0) == 0xe0 && (s[1] & 0xc0) == 0x80 &&
        (s[2] & 0xc0) == 0x80) {
        cp = ((uint32_t)(s[0] & 0x0f) << 12) |
             ((uint32_t)(s[1] & 0x3f) << 6) |
             (uint32_t)(s[2] & 0x3f);
        *text += 3;
        return cp;
    }
    if ((s[0] & 0xf8) == 0xf0 && (s[1] & 0xc0) == 0x80 &&
        (s[2] & 0xc0) == 0x80 && (s[3] & 0xc0) == 0x80) {
        cp = ((uint32_t)(s[0] & 0x07) << 18) |
             ((uint32_t)(s[1] & 0x3f) << 12) |
             ((uint32_t)(s[2] & 0x3f) << 6) |
             (uint32_t)(s[3] & 0x3f);
        *text += 4;
        return cp;
    }

    if (s[0] >= 0xc0) {
        *text += 1;
        return 0x0410 + (uint32_t)(s[0] - 0xc0);
    }
    if (s[0] >= 0x80) {
        cp = cp1251_extra[s[0] - 0x80];
        *text += 1;
        return cp != 0 ? cp : '?';
    }

    *text += 1;
    return '?';
}

static void pm_draw_glyph(SDL_Surface *dst, int x, int y, uint32_t cp, Uint32 color) {
    const unsigned char *font = pm_select_font_bitmap(cp);
    int width, height, first, glyph, row, col;
    unsigned long first_pixel, map_len;

    if (cp < 0x80) {
        characterColor(dst, (Sint16)x, (Sint16)y, (char)cp, color);
        return;
    }

    if (font == NULL) {
        characterColor(dst, (Sint16)x, (Sint16)y, '?', color);
        return;
    }

    width = font[PM_FONT_WIDTH];
    height = font[PM_FONT_HEIGHT];
    first = (font[PM_FONT_CODE_RANGE_HIGH] << 8) | font[PM_FONT_CODE_FIRST_LOW];
    glyph = (int)cp - first;
    first_pixel = (unsigned long)glyph * width * height;
    map_len = (((unsigned long)(font[PM_FONT_CODE_LAST_LOW] -
                font[PM_FONT_CODE_FIRST_LOW] + 1) * width * height) + 7) >> 3;

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            unsigned long pixel = first_pixel + (unsigned long)row * width + col;
            unsigned long byte_index = pixel >> 3;
            int bit = (int)(pixel & 7);
            if (byte_index < map_len &&
                (font[PM_FONT_DATA + byte_index] & (0x80 >> bit)) != 0 &&
                x + col >= 0 && x + col < dst->w &&
                y + row >= 0 && y + row < dst->h) {
                pixelColor(dst, (Sint16)(x + col), (Sint16)(y + row), color);
            }
        }
    }
}

static int pm_stringColor(SDL_Surface *dst, Sint16 x, Sint16 y,
                          const char *text, Uint32 color) {
    int pen_x = x;
    const char *p = text;

    if (p != NULL) {
        const unsigned char *scan = (const unsigned char *)p;
        while (*scan != '\0' && *scan < 0x80) scan++;
        if (*scan == '\0') {
            return stringColor(dst, x, y, text, color);
        }
    }

    while (p && *p) {
        uint32_t cp = pm_next_utf8(&p);
        pm_draw_glyph(dst, pen_x, y, cp, color);
        pen_x += (cp < 0x80) ? PM_ASCII_ADVANCE : PM_FONT_ADVANCE;
    }
    return 0;
}

#define stringColor pm_stringColor

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
static char *trim_line(char *text);
static int read_midlet_name_from_jad(const char *jad_path, char *out, size_t out_size);
static int read_jam_value(const char *jam_path, const char *key, char *out, size_t out_size);
static void shell_quote(const char *src, char *dst, size_t dst_size);
static int find_key_profile(const char *id);
static void enter_selected_folder(void);
static int leave_folder(void);
static const char *game_display_name(int idx);
static int is_favorite_path(const char *path);
static int fav_loaded;
static int toggle_favorite(const char *path);

/* ─────────────────────────────────────────────────────
 *  game scanning
 * ──────────────────────────────────────────────────── */
static void sanitize_config_name(char *text) {
    int i;
    for (i = 0; text[i] != '\0'; i++) {
        if (text[i] == '/') {
            text[i] = '_';
        }
    }
}

static void add_folder(const char *full_path, const char *name) {
    if (!grow_games()) return;
    games[game_count].path = strdup(full_path);
    snprintf(games[game_count].label, sizeof(games[game_count].label), "%s/", name);
    games[game_count].config[0] = '\0';
    games[game_count].title[0] = '\0';
    games[game_count].jam_path[0] = '\0';
    games[game_count].sp_path[0] = '\0';
    games[game_count].app_class[0] = '\0';
    games[game_count].app_param[0] = '\0';
    games[game_count].app_ver[0] = '\0';
    games[game_count].is_dir = 1;
    games[game_count].is_doja = 0;
    games[game_count].is_favorites_folder = 0;
    games[game_count].is_favorite = 0;
    game_count++;
}

static void add_favorites_folder(void) {
    if (!grow_games()) return;
    games[game_count].path = strdup("");
    snprintf(games[game_count].label, sizeof(games[game_count].label), "* Favorites *");
    games[game_count].config[0] = '\0';
    games[game_count].title[0] = '\0';
    games[game_count].is_dir = 1;
    games[game_count].is_favorites_folder = 1;
    games[game_count].is_favorite = 0;
    game_count++;
}

/*
 * Names of the folder being scanned, sorted case-insensitively: .jam/.jad
 * lookups become a binary search instead of stat()/opendir() per jar (the
 * "only .jam in the folder" rule used to reread the whole folder per jar).
 */
typedef struct {
    char dir[PATH_MAX];
    char **names;
    int count;
    int jam_count;
    char only_jam[PATH_MAX];
    char only_cls[256];
    int only_cls_state;          /* 0 = not read, 1 = read, -1 = none */
} DirIndex;
static DirIndex *scan_index = NULL;

static int name_cmp(const void *a, const void *b) {
    return strcasecmp(*(char * const *)a, *(char * const *)b);
}

/* actual name of <want> in the indexed folder (any case), NULL when missing */
static const char *dir_index_find(const DirIndex *ix, const char *want) {
    char **hit;
    if (ix == NULL || ix->names == NULL) return NULL;
    hit = (char **)bsearch(&want, ix->names, ix->count, sizeof(char *), name_cmp);
    return hit ? *hit : NULL;
}

/* the indexed folder when <path> is directly inside it */
static DirIndex *dir_index_for(const char *path) {
    size_t n;
    if (scan_index == NULL) return NULL;
    n = strlen(scan_index->dir);
    if (strncmp(path, scan_index->dir, n) != 0 || path[n] != '/') return NULL;
    if (strchr(path + n + 1, '/') != NULL) return NULL;
    return scan_index;
}

/* entry names of a zip read straight from its central directory */
static int jar_has_entry(const char *jar_path, const char *entry) {
    FILE *f = fopen(jar_path, "rb");
    unsigned char tail[22 + 65535];
    long size, start;
    int n, i, found = 0;
    unsigned long cd_off, cd_size, pos;
    size_t elen = strlen(entry);

    if (f == NULL) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    size = ftell(f);
    start = size > (long)sizeof(tail) ? size - (long)sizeof(tail) : 0;
    fseek(f, start, SEEK_SET);
    n = (int)fread(tail, 1, (size_t)(size - start), f);
    for (i = n - 22; i >= 0; i--) {
        if (tail[i] == 'P' && tail[i + 1] == 'K' && tail[i + 2] == 5 && tail[i + 3] == 6) break;
    }
    if (i < 0) { fclose(f); return 0; }
    cd_size = tail[i + 12] | (tail[i + 13] << 8) | (tail[i + 14] << 16) | ((unsigned long)tail[i + 15] << 24);
    cd_off = tail[i + 16] | (tail[i + 17] << 8) | (tail[i + 18] << 16) | ((unsigned long)tail[i + 19] << 24);
    if (cd_size == 0 || cd_size > 16UL * 1024 * 1024) { fclose(f); return 0; }
    {
        unsigned char *cd = (unsigned char *)malloc(cd_size);
        if (cd == NULL) { fclose(f); return 0; }
        fseek(f, (long)cd_off, SEEK_SET);
        if (fread(cd, 1, cd_size, f) != cd_size) { free(cd); fclose(f); return 0; }
        for (pos = 0; pos + 46 <= cd_size && !found; ) {
            unsigned nl, xl, cl;
            if (cd[pos] != 'P' || cd[pos + 1] != 'K' || cd[pos + 2] != 1 || cd[pos + 3] != 2) break;
            nl = cd[pos + 28] | (cd[pos + 29] << 8);
            xl = cd[pos + 30] | (cd[pos + 31] << 8);
            cl = cd[pos + 32] | (cd[pos + 33] << 8);
            if (pos + 46 + nl > cd_size) break;
            if (nl == elen && memcmp(cd + pos + 46, entry, elen) == 0) found = 1;
            pos += 46 + nl + xl + cl;
        }
        free(cd);
    }
    fclose(f);
    return found;
}

static int jar_has_class(const char *jar_path, const char *class_name) {
    char entry[300];
    int i;

    if (class_name == NULL || class_name[0] == '\0') return 0;
    for (i = 0; class_name[i] != '\0' && i < 250; i++)
        entry[i] = class_name[i] == '.' ? '/' : class_name[i];
    entry[i] = '\0';
    strcat(entry, ".class");
    return jar_has_entry(jar_path, entry);
}

/*
 * The .jam of a jar: <name>.jam next to it, or the only .jam of the folder
 * when its AppClass is inside the jar (dumps often name them differently).
 */
static int find_jam_for_jar(const char *jar_path, char *out, size_t out_size) {
    char candidate[PATH_MAX];
    char dir[PATH_MAX];
    char only[PATH_MAX] = "";
    char cls[256];
    int jar_len = strlen(jar_path), count = 0;
    const char *slash;
    DIR *d;
    struct dirent *ent;
    struct stat st;

    out[0] = '\0';
    if (jar_len < 5) return 0;
    {
        DirIndex *ix = dir_index_for(jar_path);
        if (ix != NULL) {
            const char *base = strrchr(jar_path, '/') + 1;
            char want[PATH_MAX];
            const char *hit;
            snprintf(want, sizeof(want), "%.*s.jam", (int)strlen(base) - 4, base);
            hit = dir_index_find(ix, want);
            if (hit != NULL) {
                snprintf(out, out_size, "%s/%s", ix->dir, hit);
                return 1;
            }
            if (ix->jam_count != 1) return 0;
            if (ix->only_cls_state == 0) {
                ix->only_cls_state = read_jam_value(ix->only_jam, "AppClass", ix->only_cls,
                                                    sizeof(ix->only_cls)) ? 1 : -1;
            }
            if (ix->only_cls_state < 0 || !jar_has_class(jar_path, ix->only_cls)) return 0;
            snprintf(out, out_size, "%s", ix->only_jam);
            return 1;
        }
    }
    snprintf(candidate, sizeof(candidate), "%.*s.jam", jar_len - 4, jar_path);
    if (stat(candidate, &st) == 0 && S_ISREG(st.st_mode)) {
        snprintf(out, out_size, "%s", candidate);
        return 1;
    }
    snprintf(candidate, sizeof(candidate), "%.*s.JAM", jar_len - 4, jar_path);
    if (stat(candidate, &st) == 0 && S_ISREG(st.st_mode)) {
        snprintf(out, out_size, "%s", candidate);
        return 1;
    }
    slash = strrchr(jar_path, '/');
    if (slash == NULL) return 0;
    snprintf(dir, sizeof(dir), "%.*s", (int) (slash - jar_path), jar_path);
    d = opendir(dir);
    if (d == NULL) return 0;
    while ((ent = readdir(d)) != NULL) {
        int len = strlen(ent->d_name);
        if (len > 4 && strcasecmp(ent->d_name + len - 4, ".jam") == 0) {
            count++;
            snprintf(only, sizeof(only), "%s/%s", dir, ent->d_name);
        }
    }
    closedir(d);
    if (count != 1) return 0;
    if (!read_jam_value(only, "AppClass", cls, sizeof(cls))) return 0;
    if (!jar_has_class(jar_path, cls)) return 0;
    snprintf(out, out_size, "%s", only);
    return 1;
}

static int is_ascii_text(const char *t) {
    for (; *t; t++) {
        if ((unsigned char) *t < 0x20 || (unsigned char) *t > 0x7e) return 0;
    }
    return 1;
}

static void add_game(const char *full_path, const char *name) {
    int jar_len;
    char rel_path[PATH_MAX];
    size_t java_dir_len = strlen(java_dir);

    if (!grow_games()) return;
    if (strncmp(full_path, java_dir, java_dir_len) == 0 &&
        full_path[java_dir_len] == '/') {
        snprintf(rel_path, sizeof(rel_path), "%s", full_path + java_dir_len + 1);
    } else {
        snprintf(rel_path, sizeof(rel_path), "%s", name);
    }
    games[game_count].path = strdup(full_path);
    snprintf(games[game_count].label, sizeof(games[game_count].label), "%s", name);
    snprintf(games[game_count].config, sizeof(games[game_count].config), "%s", rel_path);
    sanitize_config_name(games[game_count].config);
    games[game_count].title[0] = '\0';
    games[game_count].jam_path[0] = '\0';
    games[game_count].sp_path[0] = '\0';
    games[game_count].app_class[0] = '\0';
    games[game_count].app_param[0] = '\0';
    games[game_count].app_ver[0] = '\0';
    games[game_count].is_dir = 0;
    games[game_count].is_doja = 0;
    games[game_count].is_favorites_folder = 0;
    games[game_count].is_favorite = is_favorite_path(full_path);
    jar_len = strlen(full_path);
    if (jar_len >= 5) {
        char jad_path[PATH_MAX];
        char jam_path[PATH_MAX];
        DirIndex *ix = dir_index_for(full_path);
        snprintf(jad_path, sizeof(jad_path), "%.*s.jad", jar_len - 4, full_path);
        if (ix != NULL) {
            const char *hit = dir_index_find(ix, strrchr(jad_path, '/') + 1);
            if (hit != NULL) {
                snprintf(jad_path, sizeof(jad_path), "%s/%s", ix->dir, hit);
                read_midlet_name_from_jad(jad_path, games[game_count].title,
                                          sizeof(games[game_count].title));
            }
        } else {
            read_midlet_name_from_jad(jad_path, games[game_count].title,
                                      sizeof(games[game_count].title));
        }
        if (find_jam_for_jar(full_path, jam_path, sizeof(jam_path))
                && read_jam_value(jam_path, "AppClass", games[game_count].app_class,
                                  sizeof(games[game_count].app_class))) {
            games[game_count].is_doja = 1;
            snprintf(games[game_count].jam_path, sizeof(games[game_count].jam_path),
                     "%s", jam_path);
            snprintf(games[game_count].sp_path, sizeof(games[game_count].sp_path),
                     "%.*s.sp", jar_len - 4, full_path);
            read_jam_value(jam_path, "AppParam", games[game_count].app_param,
                           sizeof(games[game_count].app_param));
            read_jam_value(jam_path, "AppVer", games[game_count].app_ver,
                           sizeof(games[game_count].app_ver));
            /* AppName is Shift-JIS; the launcher font only has ASCII */
            {
                char app_name[100];
                if (read_jam_value(jam_path, "AppName", app_name, sizeof(app_name))
                        && is_ascii_text(app_name)) {
                    snprintf(games[game_count].title, sizeof(games[game_count].title),
                             "%s%s", platforms[PLATFORM_DOJA].tag, app_name);
                } else {
                    games[game_count].title[0] = '\0';
                }
            }
            {
                char base[PATH_MAX];
                snprintf(base, sizeof(base), "%s", games[game_count].label);
                snprintf(games[game_count].label, sizeof(games[game_count].label),
                         "%s%s", platforms[PLATFORM_DOJA].tag, base);
            }
        }
    }
    game_count++;
}

static void current_dir_path(char *out, size_t out_size) {
    if (current_rel_dir[0] == '\0') {
        snprintf(out, out_size, "%s", java_dir);
    } else {
        snprintf(out, out_size, "%s/%s", java_dir, current_rel_dir);
    }
}

static void scan_current_dir(void) {
    DIR *d;
    struct dirent *ent;
    DirIndex ix;
    char **dirs = NULL;
    char **dirs_list = NULL;
    int cap = 0, i;

    memset(&ix, 0, sizeof(ix));
    current_dir_path(ix.dir, sizeof(ix.dir));
    d = opendir(ix.dir);
    if (!d) return;

    /* one pass over the folder: names (folders flagged with a trailing '/') */
    while ((ent = readdir(d)) != NULL) {
        const char *n = ent->d_name;
        int len = strlen(n);
        int is_dir = 0;
        char *copy;

        if (strcmp(n, ".") == 0 || strcmp(n, "..") == 0) continue;
        if (ent->d_type == DT_DIR) {
            is_dir = 1;
        } else if (ent->d_type == DT_UNKNOWN || ent->d_type == DT_LNK) {
            char full[PATH_MAX];
            struct stat st;
            snprintf(full, sizeof(full), "%s/%s", ix.dir, n);
            if (stat(full, &st) != 0) continue;
            is_dir = S_ISDIR(st.st_mode);
            if (!is_dir && !S_ISREG(st.st_mode)) continue;
        } else if (ent->d_type != DT_REG) {
            continue;
        }
        if (ix.count == cap) {
            char **nn;
            cap = cap ? cap * 2 : 256;
            nn = (char **)realloc(ix.names, cap * sizeof(char *));
            if (nn == NULL) break;
            ix.names = nn;
            nn = (char **)realloc(dirs, cap * sizeof(char *));
            if (nn == NULL) break;
            dirs = nn;
        }
        copy = strdup(n);
        if (copy == NULL) break;
        dirs[ix.count] = is_dir ? copy : NULL;
        ix.names[ix.count++] = copy;
        if (!is_dir && len > 4 && strcasecmp(n + len - 4, ".jam") == 0) {
            ix.jam_count++;
            snprintf(ix.only_jam, sizeof(ix.only_jam), "%s/%s", ix.dir, n);
        }
    }
    closedir(d);

    /* ix.names: a sorted view for lookups; list keeps the folder order */
    {
        char **list = ix.names;
        ix.names = (char **)malloc((ix.count ? ix.count : 1) * sizeof(char *));
        if (ix.names != NULL) {
            memcpy(ix.names, list, ix.count * sizeof(char *));
            qsort(ix.names, ix.count, sizeof(char *), name_cmp);
        }
        dirs_list = list;
    }

    for (i = 0; i < ix.count; i++) {
        const char *n = dirs_list[i];
        int len = strlen(n);
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", ix.dir, n);
        if (dirs[i] != NULL) {
            add_folder(full, n);
        } else if (len >= 5 && strcasecmp(n + len - 4, ".jar") == 0) {
            scan_index = &ix;
            add_game(full, n);
            scan_index = NULL;
        }
    }
    free(dirs);
    for (i = 0; i < ix.count; i++) free(dirs_list[i]);
    free(dirs_list);
    free(ix.names);
}

static void scan_favorites(void) {
    FILE *favorites = fopen(FAVORITES_FILE, "r");
    char line[PATH_MAX + 2];

    if (favorites == NULL) return;
    while (fgets(line, sizeof(line), favorites) != NULL) {
        char *path = trim_line(line);
        const char *name;
        int len;
        struct stat st;

        if (path[0] == '\0' || stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        name = strrchr(path, '/');
        name = name == NULL ? path : name + 1;
        len = strlen(name);
        if (len < 5 || strcasecmp(name + len - 4, ".jar") != 0) continue;
        add_game(path, name);
    }
    fclose(favorites);
}

static const char *entry_name(const GameEntry *g) {
    if (!g->is_dir && g->title[0] != '\0') return g->title;
    return g->label;
}

static int game_cmp(const void *pa, const void *pb) {
    const GameEntry *a = (const GameEntry *)pa, *b = (const GameEntry *)pb;
    if (a->is_favorites_folder != b->is_favorites_folder)
        return a->is_favorites_folder ? -1 : 1;
    if (a->is_dir != b->is_dir) return a->is_dir ? -1 : 1;
    return strcasecmp(entry_name(a), entry_name(b));
}

static void scan_games(void) {
    int i;
    Uint32 t0 = SDL_GetTicks();

    fav_loaded = 0;
    for (i = 0; i < game_count; i++) free(games[i].path);
    game_count = 0;
    game_sel = 0;
    game_scroll = 0;

    if (browsing_favorites) {
        scan_favorites();
    } else {
        if (current_rel_dir[0] == '\0') add_favorites_folder();
        scan_current_dir();
    }

    qsort(games, game_count, sizeof(GameEntry), game_cmp);
    fprintf(stderr, "pm: scanned %s%s: %d entries in %u ms\n",
            browsing_favorites ? "favorites " : "/", current_rel_dir, game_count,
            (unsigned)(SDL_GetTicks() - t0));
}

static const char *game_display_name(int idx) {
    if (idx >= 0 && idx < game_count && games[idx].is_dir) {
        return games[idx].label;
    }
    if (idx >= 0 && idx < game_count && games[idx].title[0] != '\0') {
        return games[idx].title;
    }
    if (idx >= 0 && idx < game_count) {
        return games[idx].label;
    }
    return "";
}

static void push_browser_return(void) {
    BrowserReturn *ret;
    if (game_count <= 0 || game_sel < 0 || game_sel >= game_count) return;
    if (nav_depth >= NAV_STACK_MAX) {
        memmove(&nav_stack[0], &nav_stack[1],
                sizeof(nav_stack[0]) * (NAV_STACK_MAX - 1));
        nav_depth = NAV_STACK_MAX - 1;
    }

    ret = &nav_stack[nav_depth++];
    snprintf(ret->path, sizeof(ret->path), "%s",
             games[game_sel].path ? games[game_sel].path : "");
    ret->scroll = game_scroll;
    ret->is_favorites_folder = games[game_sel].is_favorites_folder;
}

static void restore_browser_return(const BrowserReturn *ret) {
    int i;
    if (ret == NULL || game_count <= 0) return;

    for (i = 0; i < game_count; i++) {
        if (ret->is_favorites_folder) {
            if (!games[i].is_favorites_folder) continue;
        } else if (games[i].is_favorites_folder ||
                   strcmp(games[i].path ? games[i].path : "", ret->path) != 0) {
            continue;
        }

        game_sel = i;
        game_scroll = ret->scroll;
        if (game_scroll < 0) game_scroll = 0;
        if (game_scroll > game_sel) game_scroll = game_sel;
        return;
    }
}

static int pop_browser_return(BrowserReturn *ret) {
    if (nav_depth <= 0 || ret == NULL) return 0;
    *ret = nav_stack[--nav_depth];
    return 1;
}

/*
 * Listings of the folders above the current one are kept while browsing
 * deeper, so going back (also after playing a game) shows them at once. A
 * kept listing is dropped when the folder changed (its mtime moved).
 */
#define LIST_CACHE_MAX 4
typedef struct {
    GameEntry *games;
    int count, cap;
    char rel_dir[PATH_MAX];
    int favorites;
    time_t mtime;
} ListCache;
static ListCache list_cache[LIST_CACHE_MAX];
static int list_cache_depth = 0;

static time_t folder_mtime(const char *rel_dir, int favorites) {
    char path[PATH_MAX];
    struct stat st;
    if (favorites) {
        if (stat(FAVORITES_FILE, &st) != 0) return 0;
        return st.st_mtime;
    }
    if (rel_dir[0] == '\0') snprintf(path, sizeof(path), "%s", java_dir);
    else snprintf(path, sizeof(path), "%s/%s", java_dir, rel_dir);
    if (stat(path, &st) != 0) return 0;
    return st.st_mtime;
}

static void list_free(GameEntry *g, int count) {
    int i;
    for (i = 0; i < count; i++) free(g[i].path);
    free(g);
}

/* moves the current listing onto the cache stack */
static void list_cache_push(void) {
    ListCache *c;
    if (list_cache_depth == LIST_CACHE_MAX) {
        list_free(list_cache[0].games, list_cache[0].count);
        memmove(&list_cache[0], &list_cache[1], sizeof(ListCache) * (LIST_CACHE_MAX - 1));
        list_cache_depth--;
    }
    c = &list_cache[list_cache_depth++];
    c->games = games;
    c->count = game_count;
    c->cap = games_cap;
    snprintf(c->rel_dir, sizeof(c->rel_dir), "%s", current_rel_dir);
    c->favorites = browsing_favorites;
    c->mtime = folder_mtime(current_rel_dir, browsing_favorites);
    games = NULL;
    games_cap = 0;
    game_count = 0;
}

/* brings back the listing of the folder now current; 0 = scan it */
static int list_cache_pop(void) {
    ListCache c;
    int i;
    if (list_cache_depth == 0) return 0;
    c = list_cache[--list_cache_depth];
    if (strcmp(c.rel_dir, current_rel_dir) != 0 || c.favorites != browsing_favorites ||
        c.mtime == 0 || c.mtime != folder_mtime(c.rel_dir, c.favorites)) {
        list_free(c.games, c.count);
        return 0;
    }
    list_free(games, game_count);
    games = c.games;
    game_count = c.count;
    games_cap = c.cap;
    game_sel = 0;
    game_scroll = 0;
    /* stars may have changed inside a subfolder */
    fav_loaded = 0;
    for (i = 0; i < game_count; i++) {
        if (!games[i].is_dir) games[i].is_favorite = is_favorite_path(games[i].path);
    }
    return 1;
}

static void list_cache_clear(void) {
    while (list_cache_depth > 0) {
        list_cache_depth--;
        list_free(list_cache[list_cache_depth].games, list_cache[list_cache_depth].count);
    }
}

static void enter_selected_folder(void) {
    char next[PATH_MAX];
    if (game_count <= 0 || !games[game_sel].is_dir) return;
    push_browser_return();
    if (games[game_sel].is_favorites_folder) {
        list_cache_push();
        browsing_favorites = 1;
        scan_games();
        return;
    }
    snprintf(next, sizeof(next), "%s%s%s",
             current_rel_dir, current_rel_dir[0] ? "/" : "",
             games[game_sel].label);
    list_cache_push();
    if (next[0] != '\0') {
        size_t len = strlen(next);
        if (len > 0 && next[len - 1] == '/') next[len - 1] = '\0';
    }
    snprintf(current_rel_dir, sizeof(current_rel_dir), "%s", next);
    scan_games();
}

static int leave_folder(void) {
    char *slash;
    BrowserReturn ret;
    int have_return;
    if (browsing_favorites) {
        browsing_favorites = 0;
        have_return = pop_browser_return(&ret);
        if (!list_cache_pop()) scan_games();
        if (have_return) restore_browser_return(&ret);
        return 1;
    }
    if (current_rel_dir[0] == '\0') return 0;
    have_return = pop_browser_return(&ret);
    slash = strrchr(current_rel_dir, '/');
    if (slash != NULL) {
        *slash = '\0';
    } else {
        current_rel_dir[0] = '\0';
    }
    if (!list_cache_pop()) scan_games();
    if (have_return) restore_browser_return(&ret);
    return 1;
}

/* ─────────────────────────────────────────────────────
 *  key binding config read / write
 * ──────────────────────────────────────────────────── */
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

/* favorites.txt read once per scan instead of once per game */
static char **fav_paths = NULL;
static int fav_count = 0;

static void fav_forget(void) {
    int i;
    for (i = 0; i < fav_count; i++) free(fav_paths[i]);
    free(fav_paths);
    fav_paths = NULL;
    fav_count = 0;
    fav_loaded = 0;
}

static void fav_load(void) {
    FILE *favorites;
    char line[PATH_MAX + 2];
    int cap = 0;

    fav_forget();
    fav_loaded = 1;
    favorites = fopen(FAVORITES_FILE, "r");
    if (favorites == NULL) return;
    while (fgets(line, sizeof(line), favorites) != NULL) {
        char *t = trim_line(line);
        if (t[0] == '\0') continue;
        if (fav_count == cap) {
            char **n;
            cap = cap ? cap * 2 : 32;
            n = (char **)realloc(fav_paths, cap * sizeof(char *));
            if (n == NULL) break;
            fav_paths = n;
        }
        fav_paths[fav_count++] = strdup(t);
    }
    fclose(favorites);
}

static int is_favorite_path(const char *path) {
    int i;
    if (!fav_loaded) fav_load();
    for (i = 0; i < fav_count; i++) {
        if (fav_paths[i] != NULL && strcmp(fav_paths[i], path) == 0) return 1;
    }
    return 0;
}

static int toggle_favorite(const char *path) {
    FILE *favorites;
    FILE *updated;
    char line[PATH_MAX + 2];
    char tmp_path[PATH_MAX];
    int found = 0;

    fav_loaded = 0;

    mkdir(PM_DIR, 0777);
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", FAVORITES_FILE);
    updated = fopen(tmp_path, "w");
    if (updated == NULL) return 0;

    favorites = fopen(FAVORITES_FILE, "r");
    if (favorites != NULL) {
        while (fgets(line, sizeof(line), favorites) != NULL) {
            char *saved_path = trim_line(line);
            if (saved_path[0] == '\0') continue;
            if (strcmp(saved_path, path) == 0) {
                found = 1;
                continue;
            }
            fprintf(updated, "%s\n", saved_path);
        }
        fclose(favorites);
    }
    if (!found) fprintf(updated, "%s\n", path);

    if (fclose(updated) != 0 || rename(tmp_path, FAVORITES_FILE) != 0) {
        unlink(tmp_path);
        return 0;
    }
    return 1;
}

static void remove_favorite_path(const char *path) {
    fav_loaded = 0;
    FILE *favorites;
    FILE *updated;
    char line[PATH_MAX + 2];
    char tmp_path[PATH_MAX];

    if (path == NULL || path[0] == '\0') return;
    favorites = fopen(FAVORITES_FILE, "r");
    if (favorites == NULL) return;

    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", FAVORITES_FILE);
    updated = fopen(tmp_path, "w");
    if (updated == NULL) {
        fclose(favorites);
        return;
    }

    while (fgets(line, sizeof(line), favorites) != NULL) {
        char *saved_path = trim_line(line);
        if (saved_path[0] == '\0' || strcmp(saved_path, path) == 0) continue;
        fprintf(updated, "%s\n", saved_path);
    }

    fclose(favorites);
    if (fclose(updated) != 0 || rename(tmp_path, FAVORITES_FILE) != 0) {
        unlink(tmp_path);
    }
}

static int delete_selected_game_files(void) {
    char jar_path[PATH_MAX];
    char jad_path[PATH_MAX];
    size_t len;
    int removed = 0;

    if (game_sel < 0 || game_sel >= game_count || games[game_sel].is_dir ||
        games[game_sel].path == NULL || games[game_sel].path[0] == '\0') {
        return 0;
    }

    snprintf(jar_path, sizeof(jar_path), "%s", games[game_sel].path);
    remove_favorite_path(jar_path);

    if (unlink(jar_path) == 0) removed = 1;

    len = strlen(jar_path);
    if (len > 4 && strcasecmp(jar_path + len - 4, ".jar") == 0) {
        snprintf(jad_path, sizeof(jad_path), "%.*s.jad", (int)(len - 4), jar_path);
        if (unlink(jad_path) == 0) removed = 1;
        snprintf(jad_path, sizeof(jad_path), "%.*s.JAD", (int)(len - 4), jar_path);
        if (unlink(jad_path) == 0) removed = 1;
    }

    scan_games();
    if (game_count > 0 && game_sel >= game_count) game_sel = game_count - 1;
    if (game_scroll > game_sel) game_scroll = game_sel;
    return removed;
}

static int read_midlet_name_from_jad(const char *jad_path, char *out, size_t out_size) {
    FILE *jad;
    char line[256];

    if (out_size == 0) return 0;
    out[0] = '\0';
    if (jad_path == NULL || jad_path[0] == '\0') return 0;

    jad = fopen(jad_path, "r");
    if (jad == NULL) return 0;
    while (fgets(line, sizeof(line), jad) != NULL) {
        char *colon;
        char *value;
        if (strncasecmp(line, "MIDlet-Name", 11) != 0) continue;
        colon = strchr(line, ':');
        if (colon == NULL) continue;
        value = trim_line(colon + 1);
        if (value[0] != '\0') {
            snprintf(out, out_size, "%s", value);
            fclose(jad);
            return 1;
        }
    }
    fclose(jad);
    return 0;
}

static int read_jam_value(const char *jam_path, const char *key, char *out, size_t out_size) {
    FILE *jam;
    char line[512];
    size_t key_len;

    if (out_size == 0) return 0;
    out[0] = '\0';
    if (jam_path == NULL || jam_path[0] == '\0' || key == NULL) return 0;

    jam = fopen(jam_path, "r");
    if (jam == NULL) return 0;
    key_len = strlen(key);
    while (fgets(line, sizeof(line), jam) != NULL) {
        char *value;
        char *name = trim_line(line);
        if (strncasecmp(name, key, key_len) != 0) continue;
        value = name + key_len;
        while (*value == ' ' || *value == '\t') value++;
        if (*value != '=' && *value != ':') continue;
        value = trim_line(value + 1);
        snprintf(out, out_size, "%s", value);
        fclose(jam);
        return out[0] != '\0';
    }
    fclose(jam);
    return 0;
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
        scores[2] += 5;
    }
    if (strstr(lower, "sony") || strstr(lower, "ericsson") ||
        strstr(lower, "sonyericsson") || strstr(lower, "semc")) {
        scores[3] += 5;
    }
    if (strstr(lower, "samsung") || strstr(lower, "sgh-") ||
        strstr(lower, "gt-")) {
        scores[4] += 5;
    }
    if (strstr(lower, "lg-") || strstr(lower, "lge") ||
        strstr(lower, "lg/")) {
        scores[5] += 5;
    }
    if (strstr(lower, "motorola") || strstr(lower, "mot-") ||
        strstr(lower, "v3") || strstr(lower, "razr")) {
        scores[6] += 5;
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

    {
        char jam[PATH_MAX];
        if (find_jam_for_jar(jar_path, jam, sizeof(jam)))
            return find_key_profile(platforms[PLATFORM_DOJA].key_profile);
    }

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

static int preferred_scale_ratio_for_source(int source, int fallback_ratio) {
    if (source >= 0 && source < SOURCE_COUNT) {
        const SourcePreset *preset = &source_presets[source];
        if ((preset->width == 240 && preset->height == 320) ||
            (preset->width == 320 && preset->height == 240)) {
            return 750;
        }
    }
    return fallback_ratio;
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
        if (strcmp(id, display_modes[i].id) == 0) return i;
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

static int find_fps_limit(const char *id) {
    int i;
    if (!id) return 3;
    for (i = 0; i < FPS_LIMIT_COUNT; i++) {
        if (strcmp(id, fps_limits[i].id) == 0) return i;
    }
    return 3;
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
    fps_limit_sel = 3;
    sound_volume = 100;
    strcpy(synth_id, "hybrid");
    strcpy(filter_id, "sharp");
    pan_x = pan_y = 0;
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
        if (strcmp(key, "FPS_LIMIT") == 0) {
            fps_limit_sel = find_fps_limit(value);
            continue;
        }
        if (strcmp(key, "VOLUME") == 0) {
            sound_volume = val;
            if (sound_volume < 0) sound_volume = 0;
            if (sound_volume > 100) sound_volume = 100;
            continue;
        }
        if (strcmp(key, "FILTER") == 0) {
            if (strcmp(value, "sharp") == 0 || strcmp(value, "smooth") == 0 ||
                strcmp(value, "pixel") == 0) {
                snprintf(filter_id, sizeof(filter_id), "%s", value);
            }
            continue;
        }
        if (strcmp(key, "PAN_X") == 0 || strcmp(key, "PAN_Y") == 0) {
            if (val < -1000) val = -1000;
            if (val > 1000) val = 1000;
            if (key[4] == 'X') pan_x = val; else pan_y = val;
            continue;
        }
        if (strcmp(key, "SYNTH") == 0) {
            if (strcmp(value, "hybrid") == 0 || strcmp(value, "wavetable") == 0 ||
                strcmp(value, "fm") == 0) {
                snprintf(synth_id, sizeof(synth_id), "%s", value);
            }
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
    fprintf(f, "DISPLAY_MODE=%s\n", display_modes[display_mode_sel].id);
    fprintf(f, "FPS_LIMIT=%s\n", fps_limits[fps_limit_sel].id);
    fprintf(f, "VOLUME=%d\n", sound_volume);
    fprintf(f, "SYNTH=%s\n", synth_id);
    fprintf(f, "FILTER=%s\n", filter_id);
    fprintf(f, "PAN_X=%d\n", pan_x);
    fprintf(f, "PAN_Y=%d\n", pan_y);
    fprintf(f, "PROFILE=%s\n", key_profiles[key_profile_sel].id);

    for (i = 0; i < BIND_COUNT; i++)
        fprintf(f, "%s=%d\n", bind_names[i], binds[i]);

    fclose(f);
}

/* ─────────────────────────────────────────────────────
 *  debug options (global, shared with the in-game menu)
 *
 *  Stored in DEBUG_FILE as KEY=0/1. The runtime reads the same file
 *  (PHONEME_DEBUG_CONFIG) so the in-game menu can change it; switches the
 *  runtime reads at start-up apply on the next launch.
 * ──────────────────────────────────────────────────── */
typedef struct {
    const char *key;
    const char *label;
    const char *env;        /* set to "1" when on; NULL = handled here */
    const char *hint1;
    const char *hint2;
    int def;
} DebugOption;

static const DebugOption debug_options[] = {
    { "LOG",     "LOG FILE",    NULL,                    "Write runmidlet.log",  "previous: .prev.log",  1 },
    { "KEYS",    "KEY LOG",     "PHONEME_KEY_DEBUG",     "Every button press",   "and its phone key",    0 },
    { "EVENTS",  "EVENT LOG",   "PHONEME_EVENT_DEBUG",   "Key events delivered", "to the game",          0 },
    { "DISPLAY", "DISPLAY LOG", "PHONEME_DISPLAY_DEBUG", "Screen init and the",  "scaling layout",       0 },
    { "AUDIO",   "AUDIO LOG",   "PHONEME_AUDIO_TRACE",   "Audio device rate,",   "channels and buffer",  0 },
    { "LAUNCH",  "LAUNCH LOG",  "PM_LAUNCH_TRACE",       "Settings the launcher","passes to the game",   0 },
    { "PROFILE", "PROFILER",    "PHONEME_PROFILE",       "CPU use by function,", "to the log every 10 s",0 },
    { "FPS",     "FPS COUNTER", "PHONEME_SHOW_FPS",      "Frame rate in the",    "top-left corner",      0 }
};
#define DEBUG_OPTION_COUNT ((int)(sizeof(debug_options) / sizeof(debug_options[0])))
#define DEBUG_OPT_LOG 0
#define DEBUG_OPT_FPS (DEBUG_OPTION_COUNT - 1)
#define DEBUG_ROW_VIEW  DEBUG_OPTION_COUNT
#define DEBUG_ROW_CLEAR (DEBUG_OPTION_COUNT + 1)
#define DEBUG_ROW_COUNT (DEBUG_OPTION_COUNT + 2)

static int debug_values[DEBUG_OPTION_COUNT];
static int debug_sel = 0;
static int debug_notice = 0;   /* 1 = "logs cleared" */

static void load_debug(void) {
    FILE *f;
    char line[128];
    int i;
    for (i = 0; i < DEBUG_OPTION_COUNT; i++) debug_values[i] = debug_options[i].def;
    f = fopen(DEBUG_FILE, "r");
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        for (i = 0; i < DEBUG_OPTION_COUNT; i++) {
            if (strcmp(trim_line(line), debug_options[i].key) == 0) {
                debug_values[i] = atoi(eq + 1) != 0;
            }
        }
    }
    fclose(f);
}

static void save_debug(void) {
    FILE *f;
    int i;
    mkdir(PM_DIR, 0777);
    f = fopen(DEBUG_FILE, "w");
    if (!f) return;
    for (i = 0; i < DEBUG_OPTION_COUNT; i++)
        fprintf(f, "%s=%d\n", debug_options[i].key, debug_values[i]);
    fclose(f);
}

static void apply_debug_env(void) {
    int i;
    for (i = 0; i < DEBUG_OPTION_COUNT; i++) {
        if (debug_options[i].env == NULL) continue;
        /* the runtime tests these with getenv() != NULL: unset means off */
        if (debug_values[i]) setenv(debug_options[i].env, "1", 1);
        else unsetenv(debug_options[i].env);
    }
    setenv("PHONEME_DEBUG_CONFIG", DEBUG_FILE, 1);
    setenv("PHONEME_RESTART_FLAG", RESTART_FLAG, 1);
}

/* ── log viewer ─────────────────────────────────────── */
#define LOGVIEW_COLS   29
#define LOGVIEW_MAX    3000
#define LOGVIEW_ROWS   18
#define LOGVIEW_TAIL   (64 * 1024)

static char logview_lines[LOGVIEW_MAX][LOGVIEW_COLS + 1];
static int logview_count = 0;
static int logview_top = 0;
static int logview_prev = 0;    /* 0 = last run, 1 = run before */
static int logview_return = 0;  /* state to go back to (STATE_DEBUG when 0) */
static int crash_signal = 0;    /* signal that killed the last game, 0 = none */

static void logview_push(const char *text, int len) {
    if (logview_count >= LOGVIEW_MAX) {
        memmove(logview_lines[0], logview_lines[1],
                sizeof(logview_lines[0]) * (LOGVIEW_MAX - 1));
        logview_count = LOGVIEW_MAX - 1;
    }
    memcpy(logview_lines[logview_count], text, len);
    logview_lines[logview_count][len] = '\0';
    logview_count++;
}

static void logview_load(void) {
    FILE *f = fopen(logview_prev ? LOG_PREV_FILE : LOG_FILE, "rb");
    char cur[LOGVIEW_COLS + 1];
    int len = 0, c;
    long size;

    logview_count = 0;
    logview_top = 0;
    if (!f) return;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    if (size > LOGVIEW_TAIL) {
        /* only the end matters; skip the partial first line */
        fseek(f, size - LOGVIEW_TAIL, SEEK_SET);
        while ((c = fgetc(f)) != EOF && c != '\n') {}
    } else {
        fseek(f, 0, SEEK_SET);
    }
    while ((c = fgetc(f)) != EOF) {
        if (c == '\r') continue;
        if (c == '\n') {
            logview_push(cur, len);
            len = 0;
            continue;
        }
        if (c == '\t') c = ' ';
        if (c < 32 || c > 126) c = '?';
        if (len == LOGVIEW_COLS) {      /* wrap long lines */
            logview_push(cur, len);
            len = 0;
            cur[len++] = ' ';
        }
        cur[len++] = (char)c;
    }
    if (len > 0) logview_push(cur, len);
    fclose(f);
    /* open at the end: the interesting part is usually the last lines */
    logview_top = logview_count - LOGVIEW_ROWS;
    if (logview_top < 0) logview_top = 0;
}

/* ─────────────────────────────────────────────────────
 *  draw helpers
 * ──────────────────────────────────────────────────── */
static void draw_header_color(const char *title, int y, Uint32 color) {
    stringColor(screen, 4, y, title, color);
}

static void draw_header(const char *title, int y) {
    draw_header_color(title, y, 0x7df9ffff);
}

static int draw_item(int x, int y, const char *text, int selected) {
    if (selected) {
        boxColor(screen, 0, y - 1, SCR_W - 1, y + PM_FONT_LINE_H - 2, 0x003366ff);
        stringColor(screen, x, y, text, 0x00ff88ff);
    } else {
        stringColor(screen, x, y, text, 0xffffffff);
    }
    return y + PM_FONT_LINE_H;
}

#define VISIBLE_ITEMS  17

/* ─────────────────────────────────────────────────────
 *  browser screen
 * ──────────────────────────────────────────────────── */
static void draw_browser(void) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x10, 0x18, 0x20));

    int y = 6;
    if (browsing_favorites) {
        draw_header("/Favorites", y);
    } else if (current_rel_dir[0] == '\0') {
        draw_header("phoneME Launcher", y);
    } else {
        char title[128];
        snprintf(title, sizeof(title), "/%s", current_rel_dir);
        draw_header(title, y);
    }
    y += PM_FONT_LINE_H + 1;

    if (game_count == 0) {
        if (browsing_favorites) {
            stringColor(screen, 4, y, "No favorites yet", 0xffffffff);
            stringColor(screen, 4, y + PM_FONT_LINE_H, "SELECT adds games", 0x607078ff);
            stringColor(screen, 4, y + PM_FONT_LINE_H * 2, "B:Back", 0x607078ff);
        } else {
            char path[PATH_MAX];
            current_dir_path(path, sizeof(path));
            stringColor(screen, 4, y, path, 0xff6060ff);
            stringColor(screen, 4, y + PM_FONT_LINE_H,
                        current_rel_dir[0] ? "B:back" : "Add .jar files to play",
                        0xffffffff);
        }
        SDL_Flip(screen);
        return;
    }

    /* pages of VISIBLE_ITEMS entries */
    if (game_sel < 0) game_sel = 0;
    if (game_sel >= game_count) game_sel = game_count - 1;
    game_scroll = (game_sel / VISIBLE_ITEMS) * VISIBLE_ITEMS;
    if (game_count > VISIBLE_ITEMS) {
        char pg[32];
        int w;
        snprintf(pg, sizeof(pg), "%d/%d", game_sel / VISIBLE_ITEMS + 1,
                 (game_count + VISIBLE_ITEMS - 1) / VISIBLE_ITEMS);
        w = (int)strlen(pg) * PM_FONT_ADVANCE;
        boxColor(screen, SCR_W - w - 6, 4, SCR_W - 1, 4 + PM_FONT_LINE_H, 0x101820ff);
        stringColor(screen, SCR_W - w - 2, 6, pg, 0xffd060ff);
    }

    int i;
    for (i = game_scroll; i < game_count && i < game_scroll + VISIBLE_ITEMS && y < SCR_H - 16; i++) {
        const char *name = game_display_name(i);
        char marked_name[PATH_MAX + 3];
        if (!games[i].is_dir && games[i].is_favorite) {
            snprintf(marked_name, sizeof(marked_name), "* %s", name);
            name = marked_name;
        }
        if (games[i].is_dir) {
            if (i == game_sel) {
                boxColor(screen, 0, y - 1, SCR_W - 1, y + PM_FONT_LINE_H - 2, 0x003366ff);
                stringColor(screen, 6, y, name,
                            games[i].is_favorites_folder ? PM_COLOR_FAVORITE : 0x00ff88ff);
            } else if (games[i].is_favorites_folder) {
                stringColor(screen, 6, y, name, PM_COLOR_FAVORITE);
            } else {
                stringColor(screen, 6, y, name, 0x7df9ffff);
            }
            y += PM_FONT_LINE_H;
        } else if (games[i].title[0] != '\0') {
            if (i == game_sel) {
                boxColor(screen, 0, y - 1, SCR_W - 1, y + PM_FONT_LINE_H - 2, 0x003366ff);
                stringColor(screen, 6, y, name, 0x00ff88ff);
            } else {
                stringColor(screen, 6, y, name, 0xffffffff);
            }
            y += PM_FONT_LINE_H;
        } else {
            if (i == game_sel) {
                boxColor(screen, 0, y - 1, SCR_W - 1, y + PM_FONT_LINE_H - 2, 0x003366ff);
            }
            stringColor(screen, 6, y, name, 0xff6060ff);
            y += PM_FONT_LINE_H;
        }
    }

    /* help bar */
    y = SCR_H - 30;
    stringColor(screen, 2, y, "SELECT:Fav", 0x607078ff);
    stringColor(screen, 90, y, "</>:Page", 0x607078ff);
    stringColor(screen, 158, y, "Y:Debug", 0x607078ff);
    y = SCR_H - 15;
    stringColor(screen, 2, y, "A:Run", 0x607078ff);
    stringColor(screen, 64, y, "START:Keys", 0x607078ff);
    stringColor(screen, 158, y,
                (browsing_favorites || current_rel_dir[0]) ? "B:Back" : "B:Refresh",
                0x607078ff);

    SDL_Flip(screen);
}

/* ─────────────────────────────────────────────────────
 *  keybind settings screen
 * ──────────────────────────────────────────────────── */
static void draw_keybinds(void) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x10, 0x18, 0x20));

    int y = 6;
    char buf[128];
    snprintf(buf, sizeof(buf), "Controls: %s", game_display_name(game_sel));
    draw_header(buf, y);
    y += PM_FONT_LINE_H + 1;

    if (!key_edit_mode) {
        int i;
        if (bind_sel < 0) bind_sel = SETTINGS_ROW_COUNT - 1;
        if (bind_sel >= SETTINGS_ROW_COUNT) bind_sel = 0;
        bind_scroll = 0;

        for (i = 0; i < SETTINGS_ROW_COUNT && y < SCR_H - 24; i++) {
            if (i == SETTINGS_ROW_ZOOM) {
                snprintf(buf, sizeof(buf), "ZOOM   %s", current_scale_preset()->label);
            } else if (i == SETTINGS_ROW_SIZE) {
                snprintf(buf, sizeof(buf), "SIZE   %s", source_presets[source_sel].label);
            } else if (i == SETTINGS_ROW_SCREEN) {
                snprintf(buf, sizeof(buf), "SCREEN %s", display_modes[display_mode_sel].label);
            } else if (i == SETTINGS_ROW_FPS) {
                snprintf(buf, sizeof(buf), "FPS    %s", fps_limits[fps_limit_sel].label);
            } else if (i == SETTINGS_ROW_PROFILE) {
                snprintf(buf, sizeof(buf), "PROFILE %s", key_profiles[key_profile_sel].label);
            } else if (i == SETTINGS_ROW_KEYS) {
                snprintf(buf, sizeof(buf), "KEYS   edit bindings");
            } else {
                snprintf(buf, sizeof(buf), delete_confirm && bind_sel == SETTINGS_ROW_DELETE
                         ? "DELETE press A again" : "DELETE JAR+JAD");
            }
            y = draw_item(4, y, buf, i == bind_sel);
        }

        y = SCR_H - 15;
        stringColor(screen, 4, y,
                    delete_confirm && bind_sel == SETTINGS_ROW_DELETE
                    ? "A:confirm  B:cancel" : "A:select  B:save",
                    0x607078ff);
        SDL_Flip(screen);
        return;
    }

    if (bind_sel < bind_scroll) bind_scroll = bind_sel;
    if (bind_sel >= bind_scroll + VISIBLE_ITEMS) bind_scroll = bind_sel - VISIBLE_ITEMS + 1;

    {
        int i;
        int row_count = PHONE_KEY_COUNT + KEY_EDIT_ROW_OFFSET;
        for (i = bind_scroll; i < row_count && y < SCR_H - 24; i++) {
            if (i == KEY_EDIT_PROFILE_ROW) {
                snprintf(buf, sizeof(buf), "PROFILE %s", key_profiles[key_profile_sel].label);
            } else {
                int phone_index = i - KEY_EDIT_ROW_OFFSET;
                char assigned[64];
                format_phone_bindings(phone_key_options[phone_index].code,
                                      assigned, sizeof(assigned));
                snprintf(buf, sizeof(buf), "%-10s %s",
                         phone_short_label(phone_key_options[phone_index].label),
                         assigned);
            }
            y = draw_item(4, y, buf, i == bind_sel);
        }
    }

    /* capture prompt or help bar */
    y = SCR_H - 15;
    if (capture_mode) {
        stringColor(screen, 4, y - PM_FONT_LINE_H, "Press FunKey button to bind", 0x00ff88ff);
        stringColor(screen, 4, y, "POWER:cancel", 0x607078ff);
    } else {
        stringColor(screen, 4, y, "A:bind/change  B:menu", 0x607078ff);
    }

    SDL_Flip(screen);
}

/* ─────────────────────────────────────────────────────
 *  resolution picker – first launch
 * ──────────────────────────────────────────────────── */

#define SOURCE_PRESET_COUNT (sizeof(source_presets) / sizeof(source_presets[0]))

static int source_picker_sel = 0;
static int source_picker_scroll = 0;

static void draw_resolution_picker(void) {
    int i;
    int y = 6;
    const int max_rows = 12;

    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x10, 0x18, 0x20));

    /* game name */
    if (launching_game_idx >= 0 && launching_game_idx < game_count) {
        char title[256];
        snprintf(title, sizeof(title), "Resolution for %s",
                 game_display_name(launching_game_idx));
        draw_header(title, y);
    }
    y += PM_FONT_LINE_H + 1;

    /* list source presets */
    if (source_picker_scroll < 0) source_picker_scroll = 0;
    if (source_picker_sel < source_picker_scroll)
        source_picker_scroll = source_picker_sel;
    if (source_picker_sel >= source_picker_scroll + max_rows)
        source_picker_scroll = source_picker_sel - max_rows + 1;

    for (i = source_picker_scroll;
         i < (int)SOURCE_PRESET_COUNT && i < source_picker_scroll + max_rows;
         i++) {
        int selected = (i == source_picker_sel);
        char label[64];
        snprintf(label, sizeof(label), "%s", source_presets[i].label);
        y = draw_item(4, y, label, selected);
    }
    y += 4;

    /* help bar */
    y = SCR_H - 30;
    stringColor(screen, 4, y, "UP/DOWN:choose  A:confirm",
               0x607078ff);
    y += PM_FONT_LINE_H;
    stringColor(screen, 4, y, "B:cancel", 0x607078ff);

    SDL_Flip(screen);
}

static void handle_key_resolution(SDL_KeyboardEvent *key) {
    if (!key || key->type != SDL_KEYDOWN) return;

    switch (key->keysym.sym) {
    case SDLK_UP: case SDLK_u:
        if (source_picker_sel > 0) source_picker_sel--;
        break;
    case SDLK_DOWN: case SDLK_d:
        if (source_picker_sel < (int)SOURCE_PRESET_COUNT - 1) source_picker_sel++;
        break;
    case SDLK_LEFT: case SDLK_l:
        source_picker_sel -= 6;
        if (source_picker_sel < 0) source_picker_sel = 0;
        break;
    case SDLK_RIGHT: case SDLK_r:
        source_picker_sel += 6;
        if (source_picker_sel >= (int)SOURCE_PRESET_COUNT)
            source_picker_sel = SOURCE_PRESET_COUNT - 1;
        break;
    case SDLK_b:
        /* cancel – go back to browser */
        launching_game_idx = -1;
        state = STATE_BROWSER;
        break;
    case SDLK_RETURN: case SDLK_SPACE: case SDLK_a:
        /* confirm – save chosen resolution and launch */
        if (launching_game_idx >= 0 && launching_game_idx < game_count) {
            load_binds(games[launching_game_idx].config,
                       games[launching_game_idx].path);
            source_sel = source_picker_sel;
            save_binds(games[launching_game_idx].config);
            launch_game(launching_game_idx);
            launching_game_idx = -1;
            state = crash_signal ? STATE_CRASH : STATE_BROWSER;
        }
        break;
    }
}

static int config_file_exists(const char *game_name) {
    char path[PATH_MAX];
    FILE *f;
    if (!game_name) return 0;
    snprintf(path, sizeof(path), BIND_DIR "/%s.cfg", game_name);
    f = fopen(path, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* ─────────────────────────────────────────────────────
 *  launching overlay
 * ──────────────────────────────────────────────────── */
static void draw_launching(void) {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x10, 0x18, 0x20));

    char buf[256];
    snprintf(buf, sizeof(buf), "Launching %s...", game_display_name(game_sel));
    stringColor(screen, 4, SCR_H / 2 - 5, buf, 0x00ff88ff);

    SDL_Flip(screen);
    SDL_Delay(200);
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

/*
 * The scratchpad (.sp: the game's data and saves) is used in place, next to
 * the .jar; it is created there when the game has none. Older builds worked
 * on a copy in .pm/scratchpad: that copy holds the saves, so it is moved
 * back next to the game once.
 */
/*
 * The VM cannot open a jar (or scratchpad) whose path has non-ASCII
 * characters (Japanese folder names), and ':' would split the class path:
 * such files are reached through a symlink in /tmp.
 */
static const char *ascii_path(const char *real, const char *link_name, char *buf, size_t size) {
    const unsigned char *p;
    int plain = 1;
    for (p = (const unsigned char *) real; *p; ++p) {
        if (*p >= 0x80 || *p == ':') {
            plain = 0;
            break;
        }
    }
    if (plain) return real;
    snprintf(buf, size, "/tmp/%s", link_name);
    unlink(buf);
    if (symlink(real, buf) != 0) return real;
    return buf;
}

static void prepare_doja_scratchpad(GameEntry *game, char *out, size_t out_size) {
    struct stat st;
    char old[PATH_MAX];

    if (out_size == 0) return;
    out[0] = '\0';
    if (game == NULL || !game->is_doja || game->sp_path[0] == '\0') return;

    snprintf(out, out_size, "%s", game->sp_path);
    /* jar caches of older builds (now written to /tmp) */
    {
        char dir[PATH_MAX];
        DIR *d;
        snprintf(dir, sizeof(dir), "%s/scratchpad", PM_DIR);
        d = opendir(dir);
        if (d != NULL) {
            struct dirent *ent;
            while ((ent = readdir(d)) != NULL) {
                if (strncmp(ent->d_name, "jarinflater-", 12) == 0) {
                    snprintf(old, sizeof(old), "%s/%s", dir, ent->d_name);
                    unlink(old);
                }
            }
            closedir(d);
        }
    }
    snprintf(old, sizeof(old), "%s/scratchpad/%s.sp", PM_DIR, game->config);
    if (stat(old, &st) == 0) {
        if (rename(old, out) != 0) {
            /* other file system: copy, then drop the old copy */
            copy_file(old, out);
            unlink(old);
        }
    }
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
/* returns 1 when the game asked to be restarted from its in-game menu */
static int launch_game_once(int idx) {
    const char *jar_path = games[idx].path;
    int restart = 0;

    draw_launching();

    char self_dir[PATH_MAX];
    char rms_home[PATH_MAX];
    char rms_suite_home[PATH_MAX];
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
    mkdir(RMS_DIR, 0777);

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

        char cmd[PATH_MAX * 2 + 64];
        snprintf(cmd, sizeof(cmd), "chmod +x %s/* 2>/dev/null || true", BIN_DIR);
        if (!runtime_current) system(cmd);
    }

    {
        char src[PATH_MAX], dst[PATH_MAX];
        DIR *check;
        snprintf(dst, sizeof(dst), PM_DIR "/appdb");
        check = opendir(dst);
        if (check != NULL) {
            closedir(check);
        } else {
            snprintf(src, sizeof(src), "%s/appdb", self_dir);
            copy_dir(src, dst);
        }
    }

    snprintf(rms_home, sizeof(rms_home), RMS_DIR "/%s", games[idx].config);
    mkdir(rms_home, 0777);
    snprintf(rms_suite_home, sizeof(rms_suite_home), "%s/00000000", rms_home);
    mkdir(rms_suite_home, 0777);

    /* extract MIDlet main class from JAR manifest */
    char main_class[256] = {0};
    char jad_path[PATH_MAX];
    const Platform *plat = &platforms[games[idx].is_doja ? PLATFORM_DOJA : PLATFORM_MIDP];
    {
        char stale[PATH_MAX];
        snprintf(stale, sizeof(stale), "%s/main-class", PM_DIR);
        unlink(stale);   /* never reuse the class of the previous game */
    }
    if (plat->main_class != NULL) {
        snprintf(main_class, sizeof(main_class), "%s", plat->main_class);
    } else {
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
        read_midlet_name_from_jad(jad_path, games[idx].title,
                                  sizeof(games[idx].title));
    }

    if (plat->main_class == NULL) {
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

    if (main_class[0] == '\0') strcpy(main_class, "Unknown");

    char doja_scratchpad_path[PATH_MAX] = {0};
    if (games[idx].is_doja) {
        prepare_doja_scratchpad(&games[idx], doja_scratchpad_path,
                                sizeof(doja_scratchpad_path));
        if (doja_scratchpad_path[0] != '\0') {
            char tmp[PATH_MAX];
            const char *p = ascii_path(doja_scratchpad_path, "pm-run.sp", tmp, sizeof(tmp));
            if (p != doja_scratchpad_path)
                snprintf(doja_scratchpad_path, sizeof(doja_scratchpad_path), "%s", p);
        }
    }

    /* set env vars BEFORE fork so child inherits them + system PATH etc */
    setenv("MIDP_HOME", PM_DIR, 1);
    setenv("PHONEME_RMS_HOME", rms_home, 1);
    setenv("PHONEME_HEAP_MB", plat->heap_mb, 1);
    setenv("PHONEME_ENABLE_AUDIO", "1", 1);
    setenv("PHONEME_ENABLE_GP2X_KEYS", "1", 1);
    setenv("PHONEME_KEY_PROFILE", key_profiles[key_profile_sel].id, 1);
    setenv("PHONEME_SCALE_PRESET", current_scale_preset()->id, 1);
    setenv("PHONEME_SCALE_MODE", display_modes[display_mode_sel].id, 1);
    if (games[idx].is_doja) {
        setenv("DOJA_APP_CLASS", games[idx].app_class, 1);
        setenv("DOJA_APP_PARAM", games[idx].app_param, 1);
        setenv("DOJA_APP_VER", games[idx].app_ver, 1);
        setenv("DOJA_JAM_PATH", games[idx].jam_path, 1);
        setenv("DOJA_SCRATCHPAD_PATH", doja_scratchpad_path, 1);
    } else {
        unsetenv("DOJA_APP_CLASS");
        unsetenv("DOJA_APP_PARAM");
        unsetenv("DOJA_APP_VER");
        unsetenv("DOJA_JAM_PATH");
        unsetenv("DOJA_SCRATCHPAD_PATH");
    }
    setenv("PHONEME_FPS_LIMIT", fps_limits[fps_limit_sel].id, 1);
    {
        char cfg_path[PATH_MAX];
        snprintf(cfg_path, sizeof(cfg_path), BIND_DIR "/%s.cfg", games[idx].config);
        setenv("PHONEME_CONFIG_PATH", cfg_path, 1);
    }
    {
        char volume[16];
        snprintf(volume, sizeof(volume), "%d", sound_volume);
        setenv("PHONEME_SOUND_VOLUME", volume, 1);
    }
    setenv("PHONEME_SYNTH", synth_id, 1);
    setenv("PHONEME_SCALE_FILTER", filter_id, 1);
    {
        char pan[32];
        snprintf(pan, sizeof(pan), "%d,%d", pan_x, pan_y);
        setenv("PHONEME_PAN", pan, 1);
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
    } else if (games[idx].is_doja) {
        /* Auto for an i-appli: the screen is the .jam DrawArea */
        char area[32];
        int dw = 0, dh = 0;
        if (!read_jam_value(games[idx].jam_path, "DrawArea", area, sizeof(area))
                || sscanf(area, "%dx%d", &dw, &dh) != 2 || dw <= 0 || dh <= 0
                || dw > 480 || dh > 480) {
            dw = 240;
            dh = 240;
        }
        snprintf(area, sizeof(area), "%dx%d", dw, dh);
        setenv("PHONEME_SOURCE_SIZE", area, 1);
        setenv("PHONEME_LCD_SIZE", area, 1);
    } else {
        unsetenv("PHONEME_SOURCE_SIZE");
        unsetenv("PHONEME_LCD_SIZE");
    }

    load_debug();
    apply_debug_env();
    unlink(RESTART_FLAG);

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
        /* child — stdout+stderr go to the log, or nowhere when it is off */
        int log_fd;
        if (debug_values[DEBUG_OPT_LOG]) {
            rename(LOG_FILE, LOG_PREV_FILE);
            log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        } else {
            log_fd = open("/dev/null", O_WRONLY);
        }
        if (log_fd >= 0) {
            dup2(log_fd, STDOUT_FILENO);
            dup2(log_fd, STDERR_FILENO);
            if (log_fd > 2) close(log_fd);
        }
        if (getenv("PM_LAUNCH_TRACE") != NULL) {
            printf("pm-launch trace\n");
            printf("pm-launch jar=%s\n", jar_path);
            printf("pm-launch main=%s\n", main_class);
            printf("pm-launch doja=%d app=%s param=%s ver=%s jam=%s\n",
                   games[idx].is_doja, games[idx].app_class,
                   games[idx].app_param, games[idx].app_ver,
                   games[idx].jam_path);
            printf("pm-launch doja scratchpad=%s\n",
                   getenv("DOJA_SCRATCHPAD_PATH") ? getenv("DOJA_SCRATCHPAD_PATH") : "");
            printf("pm-launch MIDP_HOME=%s\n",
                   getenv("MIDP_HOME") ? getenv("MIDP_HOME") : "");
            printf("pm-launch PHONEME_RMS_HOME=%s\n",
                   getenv("PHONEME_RMS_HOME") ? getenv("PHONEME_RMS_HOME") : "");
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
            printf("pm-launch fps limit=%s\n",
                   getenv("PHONEME_FPS_LIMIT") ? getenv("PHONEME_FPS_LIMIT") : "");
            printf("pm-launch sound volume=%s config=%s\n",
                   getenv("PHONEME_SOUND_VOLUME") ? getenv("PHONEME_SOUND_VOLUME") : "",
                   getenv("PHONEME_CONFIG_PATH") ? getenv("PHONEME_CONFIG_PATH") : "");
            printf("pm-launch key profile=%s\n", key_profiles[key_profile_sel].id);
            printf("pm-launch debug");
            for (i = 0; i < DEBUG_OPTION_COUNT; i++)
                printf(" %s=%d", debug_options[i].key, debug_values[i]);
            printf("\n");
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
            char classpath[PATH_MAX * 2 + 64];
            char prop_app[320];
            char prop_param[320];
            char prop_ver[192];
            char prop_encoding[64];
            char prop_scratchpad[PATH_MAX + 32];
            char prop_spsize[160];
            static char prop_jam[2 * 8192 + 32];
            prop_jam[0] = '\0';
            if (games[idx].is_doja) {
                FILE *jf = fopen(games[idx].jam_path, "rb");
                if (jf != NULL) {
                    static const char hexd[] = "0123456789abcdef";
                    size_t pos = (size_t)snprintf(prop_jam, sizeof(prop_jam), "-Ddoja.jam.hex=");
                    int ch;
                    while ((ch = fgetc(jf)) != EOF && pos + 3 < sizeof(prop_jam)) {
                        prop_jam[pos++] = hexd[(ch >> 4) & 15];
                        prop_jam[pos++] = hexd[ch & 15];
                    }
                    prop_jam[pos] = '\0';
                    fclose(jf);
                }
            }
            {
                char jar_link[PATH_MAX];
                const char *jp = ascii_path(jar_path, "pm-run.jar", jar_link, sizeof(jar_link));
                if (plat->support_jar != NULL) {
                    snprintf(classpath, sizeof(classpath), "%s:%s/%s",
                             jp, PM_DIR, plat->support_jar);
                } else {
                    snprintf(classpath, sizeof(classpath), "%s", jp);
                }
            }
            if (plat->encoding != NULL) {
                snprintf(prop_encoding, sizeof(prop_encoding),
                         "-Dmicroedition.encoding=%s", plat->encoding);
            } else {
                prop_encoding[0] = '\0';
            }
            if (games[idx].is_doja) {
                snprintf(prop_app, sizeof(prop_app), "-Ddoja.app.class=%s",
                         games[idx].app_class);
                snprintf(prop_param, sizeof(prop_param), "-Ddoja.app.param=%s",
                         games[idx].app_param);
                snprintf(prop_ver, sizeof(prop_ver), "-Ddoja.app.ver=%s",
                         games[idx].app_ver);
                snprintf(prop_scratchpad, sizeof(prop_scratchpad),
                         "-Ddoja.scratchpad.path=%s", doja_scratchpad_path);
                {
                    char sizes[128];
                    if (!read_jam_value(games[idx].jam_path, "SPsize", sizes, sizeof(sizes)))
                        sizes[0] = '\0';
                    snprintf(prop_spsize, sizeof(prop_spsize), "-Ddoja.spsize=%s", sizes);
                }
            } else {
                prop_spsize[0] = '\0';
                prop_app[0] = '\0';
                prop_param[0] = '\0';
                prop_ver[0] = '\0';
                prop_scratchpad[0] = '\0';
            }
        if (games[idx].is_doja) {
        char *argv[] = {
            "runMidlet",
            /* JIT: a 3.2 KB i-appli method (Cafe Far Niente) compiles to
             * broken code (bad literal load, SIGSEGV); bigger i-appli
             * methods run in the interpreter */
            "=MaxMethodToCompile3000",
            prop_app,
            prop_param,
            prop_ver,
            prop_encoding,
            prop_scratchpad,
            prop_spsize,
            prop_jam[0] ? prop_jam : "-Ddoja.jam.hex=",
            "-classpathext",
            classpath,
            "internal",
            main_class,
            NULL
        };
        execv(runmidlet_path, argv);
        } else {
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
        }
        exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        sync();
        crash_signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;

        /* re-init SDL for browser */
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
        screen = SDL_SetVideoMode(SCR_W, SCR_H, 16, SDL_SWSURFACE);
        SDL_ShowCursor(SDL_DISABLE);
        restart = (unlink(RESTART_FLAG) == 0);
    }
    return restart;
}

static void launch_game(int idx) {
    /* "Restart game" in the in-game menu: run it again with the settings
     * (and debug switches) it saved */
    while (launch_game_once(idx)) {
        load_binds(games[idx].config, games[idx].path);
    }
}

/* ─────────────────────────────────────────────────────
 *  debug screen and log viewer
 * ──────────────────────────────────────────────────── */
static void draw_debug(void) {
    char buf[64];
    int i, y = 6;
    const char *h1 = "", *h2 = "";
    Uint32 hint_color = 0x9aaab0ff;

    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x10, 0x18, 0x20));
    draw_header("Debug (all games)", y);
    y += PM_FONT_LINE_H + 1;

    for (i = 0; i < DEBUG_ROW_COUNT; i++) {
        if (i < DEBUG_OPTION_COUNT) {
            const char *value = debug_values[i] ? "On" : "Off";
            if (i != DEBUG_OPT_LOG && i != DEBUG_OPT_FPS &&
                debug_values[i] && !debug_values[DEBUG_OPT_LOG]) {
                value = "On (no log)";
            }
            snprintf(buf, sizeof(buf), "%-12s %s", debug_options[i].label, value);
        } else if (i == DEBUG_ROW_VIEW) {
            snprintf(buf, sizeof(buf), "VIEW LOG     >");
        } else {
            snprintf(buf, sizeof(buf), "CLEAR LOGS");
        }
        y = draw_item(4, y, buf, i == debug_sel);
    }

    if (debug_sel < DEBUG_OPTION_COUNT) {
        h1 = debug_options[debug_sel].hint1;
        h2 = debug_options[debug_sel].hint2;
        if (debug_sel != DEBUG_OPT_LOG && debug_sel != DEBUG_OPT_FPS &&
            !debug_values[DEBUG_OPT_LOG]) {
            h2 = "needs LOG FILE on";
        }
    } else if (debug_sel == DEBUG_ROW_VIEW) {
        h1 = "Read the log of the";
        h2 = "last game run here";
    } else if (debug_notice) {
        h1 = "Logs deleted";
        hint_color = 0x00ff88ff;
    } else {
        h1 = "Delete runmidlet.log";
        h2 = "and runmidlet.prev.log";
    }
    y += 6;
    stringColor(screen, 4, y, h1, hint_color);
    stringColor(screen, 4, y + PM_FONT_LINE_H, h2, hint_color);

    stringColor(screen, 4, SCR_H - 26, "Changes apply on launch", 0x607078ff);
    stringColor(screen, 4, SCR_H - 15, "A/<>:change  B:back", 0x607078ff);
    SDL_Flip(screen);
}

static void handle_key_debug(SDL_KeyboardEvent *kev) {
    if (kev->type != SDL_KEYDOWN) return;
    switch (kev->keysym.sym) {
    case SDLK_UP: case SDLK_u:
        debug_notice = 0;
        if (--debug_sel < 0) debug_sel = DEBUG_ROW_COUNT - 1;
        break;
    case SDLK_DOWN: case SDLK_d:
        debug_notice = 0;
        if (++debug_sel >= DEBUG_ROW_COUNT) debug_sel = 0;
        break;
    case SDLK_LEFT: case SDLK_l:
    case SDLK_RIGHT: case SDLK_r:
    case SDLK_RETURN: case SDLK_SPACE: case SDLK_a:
        if (debug_sel < DEBUG_OPTION_COUNT) {
            debug_values[debug_sel] = !debug_values[debug_sel];
            save_debug();
        } else if (debug_sel == DEBUG_ROW_VIEW) {
            if (kev->keysym.sym == SDLK_LEFT || kev->keysym.sym == SDLK_l ||
                kev->keysym.sym == SDLK_RIGHT || kev->keysym.sym == SDLK_r) break;
            logview_prev = 0;
            logview_load();
            state = STATE_LOGVIEW;
        } else if (kev->keysym.sym == SDLK_a || kev->keysym.sym == SDLK_RETURN ||
                   kev->keysym.sym == SDLK_SPACE) {
            unlink(LOG_FILE);
            unlink(LOG_PREV_FILE);
            sync();
            debug_notice = 1;
        }
        break;
    case SDLK_b: case SDLK_y: case SDLK_ESCAPE: case SDLK_q:
        save_debug();
        state = STATE_BROWSER;
        break;
    default:
        break;
    }
}

/* the game process died (crash, killed): say so instead of a silent return */
static void draw_crash(void) {
    char buf[64];
    const char *name;
    int y = 6;

    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x30, 0x10, 0x10));
    draw_header_color("Game crashed", y, 0xff6060ff);
    y += PM_FONT_LINE_H * 2;
    switch (crash_signal) {
    case SIGSEGV: name = "memory access error"; break;
    case SIGBUS:  name = "bus error"; break;
    case SIGILL:  name = "illegal instruction"; break;
    case SIGFPE:  name = "arithmetic error"; break;
    case SIGABRT: name = "aborted"; break;
    case SIGKILL: name = "killed (out of memory?)"; break;
    default:      name = "signal"; break;
    }
    stringColor(screen, 4, y, "The game closed unexpectedly:", 0xffffffff);
    y += PM_FONT_LINE_H;
    snprintf(buf, sizeof(buf), "%s (%d)", name, crash_signal);
    stringColor(screen, 4, y, buf, 0xffd060ff);
    y += PM_FONT_LINE_H * 2;
    stringColor(screen, 4, y, "The log has the details", 0x9aaab0ff);
    y += PM_FONT_LINE_H;
    stringColor(screen, 4, y, "(turn on Log file in Debug).", 0x9aaab0ff);
    stringColor(screen, 2, SCR_H - 15, "A:View log  B:Back", 0x607078ff);
    SDL_Flip(screen);
}

static void handle_key_crash(SDL_KeyboardEvent *kev) {
    if (kev->type != SDL_KEYDOWN) return;
    switch (kev->keysym.sym) {
    case SDLK_RETURN: case SDLK_SPACE: case SDLK_a:
        logview_prev = 0;
        logview_load();
        logview_return = STATE_BROWSER;
        crash_signal = 0;
        state = STATE_LOGVIEW;
        break;
    case SDLK_b: case SDLK_ESCAPE: case SDLK_q:
        crash_signal = 0;
        state = STATE_BROWSER;
        break;
    default:
        break;
    }
}

static void draw_logview(void) {
    char buf[64];
    int i, y = 6;

    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x08, 0x0c, 0x10));
    snprintf(buf, sizeof(buf), "%s  %d/%d",
             logview_prev ? "Previous run" : "Last run",
             logview_count == 0 ? 0 : logview_top + 1, logview_count);
    draw_header(buf, y);
    y += PM_FONT_LINE_H + 1;

    if (logview_count == 0) {
        stringColor(screen, 4, y, "No log yet.", 0xffffffff);
        stringColor(screen, 4, y + PM_FONT_LINE_H, "Turn LOG FILE on and", 0x9aaab0ff);
        stringColor(screen, 4, y + PM_FONT_LINE_H * 2, "run a game.", 0x9aaab0ff);
    }
    for (i = logview_top; i < logview_count && i < logview_top + LOGVIEW_ROWS; i++) {
        stringColor(screen, 4, y, logview_lines[i], 0xd0d8dcff);
        y += PM_FONT_LINE_H;
    }
    stringColor(screen, 4, SCR_H - 15, "<>:page X:prev/last B:back", 0x607078ff);
    SDL_Flip(screen);
}

static void handle_key_logview(SDL_KeyboardEvent *kev) {
    int max_top = logview_count - LOGVIEW_ROWS;
    if (kev->type != SDL_KEYDOWN) return;
    if (max_top < 0) max_top = 0;
    switch (kev->keysym.sym) {
    case SDLK_UP: case SDLK_u:       logview_top -= 1; break;
    case SDLK_DOWN: case SDLK_d:     logview_top += 1; break;
    case SDLK_LEFT: case SDLK_l:     logview_top -= LOGVIEW_ROWS; break;
    case SDLK_RIGHT: case SDLK_r:    logview_top += LOGVIEW_ROWS; break;
    case SDLK_n:                     logview_top = 0; break;         /* L1: start */
    case SDLK_m:                     logview_top = max_top; break;   /* R1: end */
    case SDLK_x:
        logview_prev = !logview_prev;
        logview_load();
        return;
    case SDLK_b: case SDLK_a: case SDLK_ESCAPE: case SDLK_q:
        state = logview_return ? logview_return : STATE_DEBUG;
        logview_return = 0;
        return;
    default:
        break;
    }
    if (logview_top > max_top) logview_top = max_top;
    if (logview_top < 0) logview_top = 0;
}

/* ─────────────────────────────────────────────────────
 *  input handlers
 * ──────────────────────────────────────────────────── */
static void handle_key_browser(SDL_KeyboardEvent *kev) {
    if (kev->type != SDL_KEYDOWN) return;

    if (kev->keysym.sym == SDLK_y) {   /* Y = debug options (global) */
        load_debug();
        debug_sel = 0;
        debug_notice = 0;
        state = STATE_DEBUG;
        return;
    }

    if (game_count <= 0) {
        switch (kev->keysym.sym) {
        case SDLK_b:
            if (!leave_folder()) scan_games();
            break;
        case SDLK_x:
        case SDLK_q:
        case SDLK_ESCAPE:
            exit(0);
        default:
            break;
        }
        return;
    }

    switch (kev->keysym.sym) {
    case SDLK_UP:    case SDLK_u:
        game_sel--;
        if (game_sel < 0) game_sel = game_count - 1;
        break;
    case SDLK_DOWN:  case SDLK_d:
        game_sel++;
        if (game_sel >= game_count) game_sel = 0;
        break;
    case SDLK_LEFT:  case SDLK_l:   /* previous page */
        game_sel -= VISIBLE_ITEMS;
        if (game_sel < 0) game_sel = 0;
        game_scroll = (game_sel / VISIBLE_ITEMS) * VISIBLE_ITEMS;
        break;
    case SDLK_RIGHT: case SDLK_r:   /* next page */
        if (game_count > 0) {
            game_sel += VISIBLE_ITEMS;
            if (game_sel >= game_count) game_sel = game_count - 1;
            game_scroll = (game_sel / VISIBLE_ITEMS) * VISIBLE_ITEMS;
        }
        break;
    case SDLK_n:                    /* L1: first entry */
        game_sel = 0;
        break;
    case SDLK_m:                    /* R1: last entry */
        if (game_count > 0) game_sel = game_count - 1;
        break;
    case SDLK_RETURN: case SDLK_SPACE: case SDLK_a: /* A = open/run */
        if (game_count > 0) {
            if (games[game_sel].is_dir) {
                enter_selected_folder();
                break;
            }
            if (!games[game_sel].is_favorites_folder &&
                !config_file_exists(games[game_sel].config)) {
                /* first launch – show resolution picker */
                source_picker_sel = 0;
                source_picker_scroll = 0;
                launching_game_idx = game_sel;
                state = STATE_RESOLUTION;
            } else {
                load_binds(games[game_sel].config, games[game_sel].path);
                launch_game(game_sel);
                state = crash_signal ? STATE_CRASH : STATE_BROWSER;
            }
        }
        break;
    case SDLK_s: /* START = open/keys */
        if (game_count > 0) {
            if (games[game_sel].is_dir) {
                enter_selected_folder();
                break;
            }
            load_binds(games[game_sel].config, games[game_sel].path);
            bind_sel = 0;
            bind_scroll = 0;
            key_edit_mode = 0;
            delete_confirm = 0;
            capture_mode = 0;
            state = STATE_KEYBINDS;
        }
        break;
    case SDLK_k: /* SELECT = add/remove favorite */
        if (game_count > 0 && !games[game_sel].is_dir) {
            if (toggle_favorite(games[game_sel].path)) {
                if (browsing_favorites) {
                    scan_games();
                } else {
                    games[game_sel].is_favorite = !games[game_sel].is_favorite;
                }
            }
        }
        break;
    case SDLK_b: /* B = back/refresh */
        if (!leave_folder()) {
            list_cache_clear();
            scan_games();
        }
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
    int row_count;
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
        phone_index = bind_sel - KEY_EDIT_ROW_OFFSET;
        if (bind_index >= 0 && phone_index >= 0 && phone_index < PHONE_KEY_COUNT) {
            binds[bind_index] = phone_key_options[phone_index].code;
        }
        capture_mode = 0;
        return;
    }

    row_count = key_edit_mode ? PHONE_KEY_COUNT + KEY_EDIT_ROW_OFFSET
                              : SETTINGS_ROW_COUNT;

    switch (kev->keysym.sym) {
    case SDLK_UP:    case SDLK_u:
        delete_confirm = 0;
        bind_sel--;
        if (bind_sel < 0) bind_sel = row_count - 1;
        break;
    case SDLK_DOWN:  case SDLK_d:
        delete_confirm = 0;
        bind_sel++;
        if (bind_sel >= row_count) bind_sel = 0;
        break;

    case SDLK_LEFT:  case SDLK_l:
        delete_confirm = 0;
        if (key_edit_mode) {
            if (bind_sel == KEY_EDIT_PROFILE_ROW) {
                key_profile_sel--;
                if (key_profile_sel < 0) key_profile_sel = KEY_PROFILE_COUNT - 1;
                apply_key_profile(key_profile_sel);
            }
        } else if (bind_sel == SETTINGS_ROW_ZOOM) {
            int scale_count;
            scale_list_for_source(source_sel, &scale_count);
            scale_sel--;
            if (scale_sel < 0) scale_sel = scale_count - 1;
        } else if (bind_sel == SETTINGS_ROW_SIZE) {
            int ratio = current_scale_preset()->ratio;
            source_sel--;
            if (source_sel < 0) source_sel = SOURCE_COUNT - 1;
            ratio = preferred_scale_ratio_for_source(source_sel, ratio);
            scale_sel = find_scale_for_source(source_sel, ratio);
        } else if (bind_sel == SETTINGS_ROW_SCREEN) {
            display_mode_sel--;
            if (display_mode_sel < 0) display_mode_sel = DISPLAY_MODE_COUNT - 1;
        } else if (bind_sel == SETTINGS_ROW_FPS) {
            fps_limit_sel--;
            if (fps_limit_sel < 0) fps_limit_sel = FPS_LIMIT_COUNT - 1;
        } else if (bind_sel == SETTINGS_ROW_PROFILE) {
            key_profile_sel--;
            if (key_profile_sel < 0) key_profile_sel = KEY_PROFILE_COUNT - 1;
            apply_key_profile(key_profile_sel);
        }
        break;
    case SDLK_RIGHT: case SDLK_r:
        delete_confirm = 0;
        if (key_edit_mode) {
            if (bind_sel == KEY_EDIT_PROFILE_ROW) {
                key_profile_sel++;
                if (key_profile_sel >= KEY_PROFILE_COUNT) key_profile_sel = 0;
                apply_key_profile(key_profile_sel);
            }
        } else if (bind_sel == SETTINGS_ROW_ZOOM) {
            int scale_count;
            scale_list_for_source(source_sel, &scale_count);
            scale_sel++;
            if (scale_sel >= scale_count) scale_sel = 0;
        } else if (bind_sel == SETTINGS_ROW_SIZE) {
            int ratio = current_scale_preset()->ratio;
            source_sel++;
            if (source_sel >= SOURCE_COUNT) source_sel = 0;
            ratio = preferred_scale_ratio_for_source(source_sel, ratio);
            scale_sel = find_scale_for_source(source_sel, ratio);
        } else if (bind_sel == SETTINGS_ROW_SCREEN) {
            display_mode_sel++;
            if (display_mode_sel >= DISPLAY_MODE_COUNT) display_mode_sel = 0;
        } else if (bind_sel == SETTINGS_ROW_FPS) {
            fps_limit_sel++;
            if (fps_limit_sel >= FPS_LIMIT_COUNT) fps_limit_sel = 0;
        } else if (bind_sel == SETTINGS_ROW_PROFILE) {
            key_profile_sel++;
            if (key_profile_sel >= KEY_PROFILE_COUNT) key_profile_sel = 0;
            apply_key_profile(key_profile_sel);
        }
        break;

    case SDLK_RETURN: case SDLK_SPACE: case SDLK_a:
        if (key_edit_mode) {
            delete_confirm = 0;
            if (bind_sel != KEY_EDIT_PROFILE_ROW) {
                capture_mode = 1;
            }
        } else if (bind_sel == SETTINGS_ROW_ZOOM) {
            int scale_count;
            scale_list_for_source(source_sel, &scale_count);
            scale_sel++;
            if (scale_sel >= scale_count) scale_sel = 0;
        } else if (bind_sel == SETTINGS_ROW_SIZE) {
            int ratio = current_scale_preset()->ratio;
            source_sel++;
            if (source_sel >= SOURCE_COUNT) source_sel = 0;
            ratio = preferred_scale_ratio_for_source(source_sel, ratio);
            scale_sel = find_scale_for_source(source_sel, ratio);
        } else if (bind_sel == SETTINGS_ROW_SCREEN) {
            display_mode_sel++;
            if (display_mode_sel >= DISPLAY_MODE_COUNT) display_mode_sel = 0;
        } else if (bind_sel == SETTINGS_ROW_FPS) {
            fps_limit_sel++;
            if (fps_limit_sel >= FPS_LIMIT_COUNT) fps_limit_sel = 0;
        } else if (bind_sel == SETTINGS_ROW_PROFILE ||
                   bind_sel == SETTINGS_ROW_KEYS) {
            delete_confirm = 0;
            key_edit_mode = 1;
            bind_sel = (bind_sel == SETTINGS_ROW_KEYS) ? KEY_EDIT_ROW_OFFSET
                                                       : KEY_EDIT_PROFILE_ROW;
            bind_scroll = 0;
        } else if (bind_sel == SETTINGS_ROW_DELETE) {
            if (!delete_confirm) {
                delete_confirm = 1;
            } else {
                delete_confirm = 0;
                delete_selected_game_files();
                state = STATE_BROWSER;
            }
        }
        break;

    case SDLK_b: /* back / cancel capture */
        if (capture_mode) {
            capture_mode = 0;
        } else if (delete_confirm) {
            delete_confirm = 0;
        } else if (key_edit_mode) {
            key_edit_mode = 0;
            bind_sel = SETTINGS_ROW_PROFILE;
            bind_scroll = 0;
        } else {
            save_binds(games[game_sel].config);
            state = STATE_BROWSER;
        }
        break;

    case SDLK_ESCAPE: case SDLK_q:
        delete_confirm = 0;
        save_binds(games[game_sel].config);
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
        case STATE_BROWSER:     draw_browser();          break;
        case STATE_KEYBINDS:    draw_keybinds();         break;
        case STATE_LAUNCHING:   draw_launching();        break;
        case STATE_RESOLUTION:  draw_resolution_picker(); break;
        case STATE_DEBUG:       draw_debug();            break;
        case STATE_LOGVIEW:     draw_logview();          break;
        case STATE_CRASH:       draw_crash();            break;
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
                case STATE_RESOLUTION:
                    handle_key_resolution(&ev.key);
                    break;
                case STATE_DEBUG:
                    handle_key_debug(&ev.key);
                    break;
                case STATE_LOGVIEW:
                    handle_key_logview(&ev.key);
                    break;
                case STATE_CRASH:
                    handle_key_crash(&ev.key);
                    break;
                }
                if (ev.type == SDL_KEYDOWN) break; /* one event per frame */
            }
        }
    }

    return 0;
}
