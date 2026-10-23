/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <kni.h>
#include <midp_logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lfjport_export.h>
#include <gxj_putpixel.h>
#include <gxj_screen_buffer.h>

#include "SDL.h"
#include "midp_constants_data.h"
#include "lfjport_scale.h"

extern int characterColor(SDL_Surface *dst, Sint16 x, Sint16 y, char c,
                          Uint32 color);

/* color is 0xRRGGBBAA like SDL_gfx. The old version passed it straight to
 * SDL_FillRect as a raw RGB565 pixel: no blending and wrong colors, so the
 * "dim" layer was a solid blue screen hiding the game behind the menu. */
static int boxColor(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2,
                    Uint32 color) {
    Uint8 r = (Uint8) (color >> 24), g = (Uint8) (color >> 16);
    Uint8 b = (Uint8) (color >> 8), a = (Uint8) color;
    int x, y;
    if (dst == NULL) return -1;
    if (x1 > x2) { Sint16 t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { Sint16 t = y1; y1 = y2; y2 = t; }
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= dst->w) x2 = (Sint16) (dst->w - 1);
    if (y2 >= dst->h) y2 = (Sint16) (dst->h - 1);
    if (x1 > x2 || y1 > y2) return 0;
    if (a == 255 || dst->format->BytesPerPixel != 2) {
        SDL_Rect rect;
        rect.x = x1;
        rect.y = y1;
        rect.w = (Uint16) (x2 - x1 + 1);
        rect.h = (Uint16) (y2 - y1 + 1);
        return SDL_FillRect(dst, &rect, SDL_MapRGB(dst->format, r, g, b));
    }
    if (SDL_MUSTLOCK(dst) && SDL_LockSurface(dst) != 0) return -1;
    {
        /* RGB565 blend */
        int sr = r >> 3, sg = g >> 2, sb = b >> 3, ia = 255 - a;
        for (y = y1; y <= y2; y++) {
            Uint16 *p = (Uint16 *) ((Uint8 *) dst->pixels + y * dst->pitch) + x1;
            for (x = x1; x <= x2; x++, p++) {
                int d = *p;
                int dr = (d >> 11) & 31, dg = (d >> 5) & 63, db = d & 31;
                dr = (sr * a + dr * ia) / 255;
                dg = (sg * a + dg * ia) / 255;
                db = (sb * a + db * ia) / 255;
                *p = (Uint16) ((dr << 11) | (dg << 5) | db);
            }
        }
    }
    if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
    return 0;
}

static int rectangleColor(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2,
                          Sint16 y2, Uint32 color) {
    boxColor(dst, x1, y1, x2, y1, color);
    boxColor(dst, x1, y2, x2, y2, color);
    boxColor(dst, x1, y1, x1, y2, color);
    return boxColor(dst, x2, y1, x2, y2, color);
}

static int stringColor(SDL_Surface *dst, Sint16 x, Sint16 y, const char *s,
                       Uint32 color) {
    Sint16 pen_x = x;
    if (dst == NULL || s == NULL) {
        return -1;
    }
    while (*s != '\0') {
        if (*s == '\n') {
            pen_x = x;
            y = (Sint16)(y + 8);
        } else {
            characterColor(dst, pen_x, y, *s, color);
            pen_x = (Sint16)(pen_x + 8);
        }
        ++s;
    }
    return 0;
}

#define SDL_FULLWIDTH	FULLWIDTH
#define SDL_FULLHEIGHT	FULLHEIGHT
void InitGP2XKeys(void);
int  InitAudioSubsystem(void);
void FinalizeAudioSubsystem(void);
void MediaSDL_SetMasterVolume(int volume);
int  MediaSDL_GetMasterVolume(void);
void MediaSDL_PauseAll(int paused);
void MediaSDL_SetSynth(int engine);
int  MediaSDL_GetSynth(void);
int  PhoneMEInputGetBind(int index);
void PhoneMEInputSetBind(int index, int key);
int  PhoneMEInputGetProfileCount(void);
const char *PhoneMEInputGetProfileId(int index);
const char *PhoneMEInputGetProfileLabel(int index);
int  PhoneMEInputGetProfileIndex(void);
void PhoneMEProfileInit(void);
void PhoneMEInputApplyProfile(int index);

SDL_Surface     *Native_SDL_Screen, *Native_SDL_HScreen, *Native_SDL_VScreen;
SDL_Surface     *Native_SDL_PScreen;
static jboolean  Native_SDL_ScreenOrientation;
static jboolean  Native_SDL_Fullscreen;
static int       OriginalOrientation, OriginalWidth, OriginalHeight;
static int       PresentScaleMode;
static int       PresentScaleRatio;
static int       PresentSourceWidth;
static int       PresentSourceHeight;
static int       PresentSourceFull;
static int       PresentFilter = SCALE_FILTER_SHARP;
static int       PresentPanX, PresentPanY;   /* -1000..1000, 0 = centered */
static int       PresentSlackX, PresentSlackY; /* last frame: was there anything to pan */
static Scaler    PresentScaler;
static int       DisplayDebug;
static int       RefreshBoundsValid;
static int       RefreshMinX, RefreshMinY, RefreshMaxX, RefreshMaxY;

enum {
    PRESENT_SCALE_AUTO = 0,
    PRESENT_SCALE_FILL,
    PRESENT_SCALE_CENTER,
    PRESENT_SCALE_FIT,
    PRESENT_SCALE_CROP,
    PRESENT_SCALE_ZOOM,
    PRESENT_SCALE_INTEGER
};

typedef struct {
    int srcX;
    int srcY;
    int srcW;
    int srcH;
    int dstX;
    int dstY;
    int dstW;
    int dstH;
    /* region the visible part may move within (pan) */
    int areaX;
    int areaY;
    int areaW;
    int areaH;
    const char *name;
} PresentLayout;

typedef struct {
    const char *id;
    const char *label;
    int mode;
} OverlayModePreset;

typedef struct {
    const char *id;
    const char *label;
    int ratio;
} OverlayScalePreset;

typedef struct {
    const char *id;
    const char *label;
    int width;
    int height;
    int full;
} OverlaySourcePreset;

typedef struct {
    const char *id;
    const char *label;
    int code;
} OverlayPhoneKey;

typedef struct {
    const char *id;
    const char *label;
    int fps;
} OverlayFpsPreset;

static const OverlayModePreset OverlayModes[] = {
    { "auto", "Auto", PRESENT_SCALE_AUTO },
    { "fit", "Fit all", PRESENT_SCALE_FIT },
    { "fill", "Stretch", PRESENT_SCALE_FILL },
    { "crop", "Crop fill", PRESENT_SCALE_CROP },
    { "center", "Center", PRESENT_SCALE_CENTER },
    { "zoom", "Manual zoom", PRESENT_SCALE_ZOOM },
    { "integer", "Integer", PRESENT_SCALE_INTEGER }
};
static const struct {
    const char *id;
    const char *label;
    int filter;
} OverlayFilters[] = {
    { "sharp", "Sharp", SCALE_FILTER_SHARP },
    { "smooth", "Smooth", SCALE_FILTER_SMOOTH },
    { "pixel", "Pixel", SCALE_FILTER_PIXEL }
};
#define OVERLAY_FILTER_COUNT ((int) (sizeof(OverlayFilters) / sizeof(OverlayFilters[0])))
static const OverlayScalePreset OverlayScales[] = {
    { "zoom-075", "75%", 750 },
    { "zoom-100", "100%", 1000 },
    { "zoom-120", "120%", 1200 },
    { "zoom-140", "140%", 1400 },
    { "zoom-150", "150%", 1500 },
    { "zoom-160", "160%", 1600 },
    { "zoom-170", "170%", 1700 },
    { "zoom-180", "180%", 1800 },
    { "zoom-185", "185%", 1850 }
};
static const OverlayScalePreset OverlayScales128x128[] = {
    { "zoom-150", "150%", 1500 },
    { "zoom-175", "175%", 1750 },
    { "zoom-1875", "188%", 1875 },
    { "zoom-200", "200%", 2000 }
};
static const OverlayScalePreset OverlayScales128x160[] = {
    { "zoom-140", "140%", 1400 },
    { "zoom-150", "150%", 1500 },
    { "zoom-160", "160%", 1600 },
    { "zoom-170", "170%", 1700 }
};
static const OverlayScalePreset OverlayScales176x208[] = {
    { "zoom-100", "100%", 1000 },
    { "zoom-110", "110%", 1100 },
    { "zoom-115", "115%", 1150 },
    { "zoom-120", "120%", 1200 }
};
static const OverlayScalePreset OverlayScales176x220[] = {
    { "zoom-100", "100%", 1000 },
    { "zoom-105", "105%", 1050 },
    { "zoom-109", "109%", 1090 },
    { "zoom-115", "115%", 1150 },
    { "zoom-120", "120%", 1200 }
};
static const OverlayScalePreset OverlayScales208x208[] = {
    { "zoom-100", "100%", 1000 },
    { "zoom-110", "110%", 1100 },
    { "zoom-115", "115%", 1150 }
};
static const OverlayScalePreset OverlayScales240x320[] = {
    { "zoom-070", "Safe 70%", 700 },
    { "zoom-075", "Fit all 75%", 750 },
    { "zoom-080", "Crop 80%", 800 },
    { "zoom-085", "Crop 85%", 850 },
    { "zoom-090", "Crop 90%", 900 },
    { "zoom-100", "Full width", 1000 }
};
static const OverlayScalePreset OverlayScales320x240[] = {
    { "zoom-070", "Safe 70%", 700 },
    { "zoom-075", "Fit all 75%", 750 },
    { "zoom-080", "Crop 80%", 800 },
    { "zoom-085", "Crop 85%", 850 },
    { "zoom-090", "Crop 90%", 900 },
    { "zoom-100", "Full height", 1000 }
};
static const OverlaySourcePreset OverlaySources[] = {
    { "auto", "Auto", 0, 0, 0 },
    { "full", "Full buffer", 0, 0, 1 },
    { "128x128", "128x128", 128, 128, 0 },
    { "128x160", "128x160", 128, 160, 0 },
    { "176x208", "176x208", 176, 208, 0 },
    { "176x220", "176x220", 176, 220, 0 },
    { "208x208", "208x208", 208, 208, 0 },
    { "240x320", "Tall 240x320", 240, 320, 0 },
    { "320x240", "Wide 320x240", 320, 240, 0 }
};
static const OverlayFpsPreset OverlayFpsLimits[] = {
    { "off", "Off", 0 },
    { "20", "20 FPS", 20 },
    { "25", "25 FPS", 25 },
    { "30", "30 FPS", 30 },
    { "50", "50 FPS", 50 },
    { "60", "60 FPS", 60 }
};
static const OverlayPhoneKey OverlayPhoneKeys[] = {
    { "up", "PHONE UP", -1 }, { "down", "PHONE DOWN", -2 },
    { "left", "PHONE LEFT", -3 }, { "right", "PHONE RIGHT", -4 },
    { "fire", "PHONE FIRE", -5 },
    { "soft1", "SOFT L NOK -6", -6 },
    { "soft2", "SOFT R NOK -7", -7 },
    { "soft_l_21", "SOFT L 21", 21 },
    { "soft_r_22", "SOFT R 22", 22 },
    { "soft_l_moto", "SOFT L MOTO -21", -21 },
    { "soft_r_moto", "SOFT R MOTO -22", -22 },
    { "clear", "PHONE CLEAR", -8 },
    { "send", "PHONE SEND", -10 }, { "end", "PHONE END", -11 },
    { "gamea", "PHONE GAMEA", -13 }, { "gameb", "PHONE GAMEB", -14 },
    { "gamec", "PHONE GAMEC", -15 }, { "gamed", "PHONE GAMED", -16 },
    { "gameup", "PHONE GAME UP", -17 }, { "gamedown", "PHONE GAME DOWN", -18 },
    { "gameleft", "PHONE GAME LEFT", -19 }, { "gameright", "PHONE GAME RIGHT", -20 },
    { "1", "PHONE 1", 49 }, { "2", "PHONE 2", 50 },
    { "3", "PHONE 3", 51 }, { "4", "PHONE 4", 52 },
    { "5", "PHONE 5", 53 }, { "6", "PHONE 6", 54 },
    { "7", "PHONE 7", 55 }, { "8", "PHONE 8", 56 },
    { "9", "PHONE 9", 57 }, { "0", "PHONE 0", 48 },
    { "star", "PHONE *", 42 }, { "pound", "PHONE #", 35 }
};
static const char *OverlayBindNames[] = {
    "UP", "DOWN", "LEFT", "RIGHT", "A", "B", "X", "Y",
    "L1", "R1", "START", "SELECT"
};
static const char *OverlayBindLabels[] = {
    "UP", "DOWN", "LEFT", "RIGHT", "A", "B", "X", "Y",
    "L1", "R1", "START", "SELECT"
};

#define OVERLAY_MODE_COUNT ((int)(sizeof(OverlayModes) / sizeof(OverlayModes[0])))
#define OVERLAY_SCALE_COUNT ((int)(sizeof(OverlayScales) / sizeof(OverlayScales[0])))
#define OVERLAY_SOURCE_COUNT ((int)(sizeof(OverlaySources) / sizeof(OverlaySources[0])))
#define OVERLAY_FPS_LIMIT_COUNT ((int)(sizeof(OverlayFpsLimits) / sizeof(OverlayFpsLimits[0])))
#define OVERLAY_PHONE_COUNT ((int)(sizeof(OverlayPhoneKeys) / sizeof(OverlayPhoneKeys[0])))
#define OVERLAY_BIND_COUNT ((int)(sizeof(OverlayBindNames) / sizeof(OverlayBindNames[0])))
#define OVERLAY_CONTROL_ROW_OFFSET 1

static int OverlayVisible;
static int OverlayPage;
static int OverlayRow;
static int OverlayCapture;
static int OverlayShutdownRequested;
static int OverlayModeSel;
static int OverlayScaleSel;
static int OverlaySourceSel;
static int OverlayResolutionChanged;
static int OverlayFpsSel = 3;
static int OverlayProfileSel;
static int OverlayVolume = 100;
static int OverlaySynthSel;      /* index into OverlaySynths */
static const char *OverlayConfigPath;
static int FrameLimitFps = 30;
/* Deadline of the next frame in microseconds of SDL_GetTicks time; 0 means
 * "start a new schedule" (after start-up, a limit change or the menu). */
static unsigned long long FrameLimitNext;

/* MIDI synth presets (fkaudio engine ids: 1 wavetable, 2 FM, 3 hybrid) */
static const struct {
    const char *id;
    const char *label;
    int engine;
} OverlaySynths[] = {
    { "hybrid",    "Hybrid",    3 },
    { "wavetable", "Wavetable", 1 },
    { "fm",        "FM",        2 }
};
#define OVERLAY_SYNTH_COUNT ((int) (sizeof(OverlaySynths) / sizeof(OverlaySynths[0])))

enum {
    OVERLAY_PAGE_MAIN = 0,
    OVERLAY_PAGE_DISPLAY,
    OVERLAY_PAGE_SOUND,
    OVERLAY_PAGE_CONTROLS,
    OVERLAY_PAGE_VIEW,         /* "Adjust view": move/zoom over the frozen game */
    OVERLAY_PAGE_DEBUG
};

enum {
    OVERLAY_MAIN_RESUME = 0,
    OVERLAY_MAIN_DISPLAY,
    OVERLAY_MAIN_CONTROLS,
    OVERLAY_MAIN_SOUND,
    OVERLAY_MAIN_DEBUG,
    OVERLAY_MAIN_RESTART,
    OVERLAY_MAIN_EXIT,
    OVERLAY_MAIN_COUNT
};

/*
 * Debug switches, shared with the launcher through PHONEME_DEBUG_CONFIG
 * (KEY=0/1 lines). Everything except the FPS counter is read at start-up,
 * so those changes apply when the game is restarted.
 */
static const struct {
    const char *key;
    const char *label;
    int def;
    int live;
} OverlayDebugOptions[] = {
    { "LOG",     "Log file",    1, 0 },
    { "KEYS",    "Key log",     0, 0 },
    { "EVENTS",  "Event log",   0, 0 },
    { "DISPLAY", "Display log", 0, 0 },
    { "AUDIO",   "Audio log",   0, 0 },
    { "LAUNCH",  "Launch log",  0, 0 },
    { "PROFILE", "Profiler",    0, 0 },
    { "FPS",     "FPS counter", 0, 1 }
};
#define OVERLAY_DEBUG_COUNT ((int) (sizeof(OverlayDebugOptions) / sizeof(OverlayDebugOptions[0])))
#define OVERLAY_DEBUG_FPS   (OVERLAY_DEBUG_COUNT - 1)
#define OVERLAY_DEBUG_ROW_RESTART OVERLAY_DEBUG_COUNT

static const char *OverlayDebugPath;
static int OverlayDebug[OVERLAY_DEBUG_COUNT];
static int OverlayDebugAtStart[OVERLAY_DEBUG_COUNT];

/* FPS counter */
static int ShowFps;
static int FpsFrames;
static int FpsValue = -1;
static Uint32 FpsWindowStart;

static int parse_screen_size(const char *text, int *width, int *height) {
    char *endptr;
    long parsedWidth, parsedHeight;

    if (text == NULL || *text == '\0') {
        return 0;
    }

    parsedWidth = strtol(text, &endptr, 10);
    if (endptr == text || *endptr != 'x') {
        return 0;
    }

    parsedHeight = strtol(endptr + 1, &endptr, 10);
    if (*endptr != '\0') {
        return 0;
    }

    if (parsedWidth <= 0 || parsedHeight <= 0) {
        return 0;
    }

    *width = (int)parsedWidth;
    *height = (int)parsedHeight;
    return 1;
}

static int find_fps_limit_index(const char *id) {
    int i;
    if (id == NULL || id[0] == '\0') return 3;
    for (i = 0; i < OVERLAY_FPS_LIMIT_COUNT; i++) {
        if (strcmp(id, OverlayFpsLimits[i].id) == 0) return i;
    }
    return 3;
}

static void configure_frame_limit(void) {
    const char *fpsText = getenv("PHONEME_FPS_LIMIT");
    OverlayFpsSel = find_fps_limit_index(fpsText);
    FrameLimitFps = OverlayFpsLimits[OverlayFpsSel].fps;
    FrameLimitNext = 0;
}

/*
 * Frames are released on a fixed grid of 1/FPS seconds instead of "at least
 * 1/FPS after the previous frame". With uneven frame costs (typical for 3D:
 * 5 ms, 45 ms, 5 ms, ...) the old rule waited after every fast frame even when
 * the slow one had already used up the time, so a game that could do 40 FPS
 * got 25 at a 30 limit. A late frame may now be made up by the next ones, up
 * to 50 ms (or two frame periods) of lag. Larger lag (loading, the menu, a
 * game that simply cannot reach the limit) is capped at that amount rather
 * than reset, so a stall never turns into a burst and an overloaded game is
 * never delayed.
 */
static void wait_frame_limit(void) {
    unsigned long long now;
    unsigned long long frameUs;
    unsigned long long maxLag;

    if (FrameLimitFps <= 0) {
        FrameLimitNext = 0;
        return;
    }
    frameUs = 1000000ULL / (unsigned long long)FrameLimitFps;
    maxLag = frameUs * 2ULL > 50000ULL ? frameUs * 2ULL : 50000ULL;
    now = (unsigned long long)SDL_GetTicks() * 1000ULL;
    if (FrameLimitNext == 0) {
        FrameLimitNext = now;            /* start the schedule */
    } else if (now > FrameLimitNext + maxLag) {
        FrameLimitNext = now - maxLag;   /* forget lag beyond the budget */
    } else if (now < FrameLimitNext) {
        SDL_Delay((Uint32)((FrameLimitNext - now + 999ULL) / 1000ULL));
    }
    FrameLimitNext += frameUs;
}

static void configure_logical_screen_size(void) {
    const char *sizeText;
    int configuredWidth, configuredHeight;

    sizeText = getenv("PHONEME_LCD_SIZE");
    if (parse_screen_size(sizeText, &configuredWidth, &configuredHeight)) {
        OriginalWidth = configuredWidth;
        OriginalHeight = configuredHeight;
        if (DisplayDebug) {
            fprintf(stderr, "lfjport_ui_init: PHONEME_LCD_SIZE=%s -> logical=%dx%d\n",
                    sizeText, OriginalWidth, OriginalHeight);
        }
    } else {
        if (DisplayDebug) {
            fprintf(stderr, "lfjport_ui_init: logical=%dx%d (default)\n",
                    OriginalWidth, OriginalHeight);
        }
    }
}

static const char *present_scale_mode_name(void) {
    switch (PresentScaleMode) {
    case PRESENT_SCALE_FILL:   return "fill";
    case PRESENT_SCALE_CENTER: return "center";
    case PRESENT_SCALE_FIT:    return "fit";
    case PRESENT_SCALE_CROP:   return "crop";
    case PRESENT_SCALE_ZOOM:   return "zoom";
    case PRESENT_SCALE_INTEGER: return "integer";
    default:                   return "auto";
    }
}

static void configure_present_scale_mode(void) {
    const char *modeText;
    const char *ratioText;
    const char *sourceText;
    char sourceDesc[32];
    int ratio;

    PresentScaleMode = PRESENT_SCALE_AUTO;
    PresentScaleRatio = 1000;
    PresentSourceWidth = 0;
    PresentSourceHeight = 0;
    PresentSourceFull = 0;
    modeText = getenv("PHONEME_SCALE_MODE");
    if (modeText != NULL) {
        if (strcmp(modeText, "fill") == 0) {
            PresentScaleMode = PRESENT_SCALE_FILL;
        } else if (strcmp(modeText, "center") == 0 || strcmp(modeText, "none") == 0) {
            PresentScaleMode = PRESENT_SCALE_CENTER;
        } else if (strcmp(modeText, "fit") == 0) {
            PresentScaleMode = PRESENT_SCALE_FIT;
        } else if (strcmp(modeText, "crop") == 0) {
            PresentScaleMode = PRESENT_SCALE_CROP;
        } else if (strcmp(modeText, "zoom") == 0) {
            PresentScaleMode = PRESENT_SCALE_ZOOM;
        } else if (strcmp(modeText, "integer") == 0) {
            PresentScaleMode = PRESENT_SCALE_INTEGER;
        }
    }
    {
        const char *filterText = getenv("PHONEME_SCALE_FILTER");
        const char *panText = getenv("PHONEME_PAN");
        int i;
        PresentFilter = SCALE_FILTER_SHARP;
        for (i = 0; filterText != NULL && i < OVERLAY_FILTER_COUNT; i++) {
            if (strcmp(filterText, OverlayFilters[i].id) == 0) PresentFilter = OverlayFilters[i].filter;
        }
        PresentPanX = PresentPanY = 0;
        if (panText != NULL && sscanf(panText, "%d,%d", &PresentPanX, &PresentPanY) == 2) {
            if (PresentPanX < -1000) PresentPanX = -1000;
            if (PresentPanX > 1000) PresentPanX = 1000;
            if (PresentPanY < -1000) PresentPanY = -1000;
            if (PresentPanY > 1000) PresentPanY = 1000;
        } else {
            PresentPanX = PresentPanY = 0;
        }
    }

    ratioText = getenv("PHONEME_SCALE_RATIO");
    if (ratioText != NULL) {
        ratio = atoi(ratioText);
        if (ratio >= 250 && ratio <= 4000) {
            PresentScaleRatio = ratio;
        } else if (ratio >= 25 && ratio <= 400) {
            PresentScaleRatio = ratio * 10;
        }
    }
    sourceText = getenv("PHONEME_SOURCE_SIZE");
    if (sourceText != NULL && strcmp(sourceText, "full") == 0) {
        PresentSourceFull = 1;
    } else {
        parse_screen_size(sourceText, &PresentSourceWidth, &PresentSourceHeight);
    }
    if (PresentSourceFull) {
        snprintf(sourceDesc, sizeof(sourceDesc), "full");
    } else {
        snprintf(sourceDesc, sizeof(sourceDesc), "%dx%d",
                 PresentSourceWidth, PresentSourceHeight);
    }

    if (DisplayDebug) {
        fprintf(stderr, "lfjport_ui_init: PHONEME_SCALE_MODE=%s ratio=%d source=%s -> present=%s\n",
                modeText != NULL ? modeText : "",
                PresentScaleRatio,
                sourceDesc,
                present_scale_mode_name());
    }
}

static int overlay_find_mode(int mode) {
    int i;
    for (i = 0; i < OVERLAY_MODE_COUNT; i++) {
        if (OverlayModes[i].mode == mode) return i;
    }
    return 0;
}

static int overlay_find_source(void) {
    int i;
    if (PresentSourceFull) return 1;
    for (i = 0; i < OVERLAY_SOURCE_COUNT; i++) {
        if (!OverlaySources[i].full &&
            OverlaySources[i].width == PresentSourceWidth &&
            OverlaySources[i].height == PresentSourceHeight) {
            return i;
        }
    }
    return 0;
}

static const OverlayScalePreset *overlay_scale_list_for_source(int source, int *count) {
    if (source >= 0 && source < OVERLAY_SOURCE_COUNT) {
        const OverlaySourcePreset *preset = &OverlaySources[source];
        if (preset->width == 128 && preset->height == 128) {
            *count = (int)(sizeof(OverlayScales128x128) / sizeof(OverlayScales128x128[0]));
            return OverlayScales128x128;
        }
        if (preset->width == 128 && preset->height == 160) {
            *count = (int)(sizeof(OverlayScales128x160) / sizeof(OverlayScales128x160[0]));
            return OverlayScales128x160;
        }
        if (preset->width == 176 && preset->height == 208) {
            *count = (int)(sizeof(OverlayScales176x208) / sizeof(OverlayScales176x208[0]));
            return OverlayScales176x208;
        }
        if (preset->width == 176 && preset->height == 220) {
            *count = (int)(sizeof(OverlayScales176x220) / sizeof(OverlayScales176x220[0]));
            return OverlayScales176x220;
        }
        if (preset->width == 208 && preset->height == 208) {
            *count = (int)(sizeof(OverlayScales208x208) / sizeof(OverlayScales208x208[0]));
            return OverlayScales208x208;
        }
        if (preset->width == 240 && preset->height == 320) {
            *count = (int)(sizeof(OverlayScales240x320) / sizeof(OverlayScales240x320[0]));
            return OverlayScales240x320;
        }
        if (preset->width == 320 && preset->height == 240) {
            *count = (int)(sizeof(OverlayScales320x240) / sizeof(OverlayScales320x240[0]));
            return OverlayScales320x240;
        }
    }

    *count = OVERLAY_SCALE_COUNT;
    return OverlayScales;
}

static const OverlayScalePreset *overlay_current_scale(void) {
    int count;
    const OverlayScalePreset *list = overlay_scale_list_for_source(OverlaySourceSel, &count);
    if (OverlayScaleSel < 0) OverlayScaleSel = 0;
    if (OverlayScaleSel >= count) OverlayScaleSel = count - 1;
    return &list[OverlayScaleSel];
}

static int overlay_find_scale_for_source(int source, int ratio) {
    int count;
    int i;
    int best = 0;
    int bestDiff;
    const OverlayScalePreset *list = overlay_scale_list_for_source(source, &count);
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

static int overlay_preferred_scale_ratio_for_source(int source, int fallbackRatio) {
    if (source >= 0 && source < OVERLAY_SOURCE_COUNT) {
        const OverlaySourcePreset *preset = &OverlaySources[source];
        if ((preset->width == 240 && preset->height == 320) ||
            (preset->width == 320 && preset->height == 240)) {
            return 750;
        }
    }
    return fallbackRatio;
}

static int overlay_physical_bind_index(SDLKey key) {
    switch (key) {
    case SDLK_u: case SDLK_UP: return 0;
    case SDLK_d: case SDLK_DOWN: return 1;
    case SDLK_l: case SDLK_LEFT: return 2;
    case SDLK_r: case SDLK_RIGHT: return 3;
    case SDLK_a: case SDLK_RETURN: case SDLK_SPACE: return 4;
    case SDLK_b: return 5;
    case SDLK_x: return 6;
    case SDLK_y: return 7;
    case SDLK_n: return 8;
    case SDLK_m: return 9;
    case SDLK_s: return 10;
    case SDLK_k: return 11;
    default: return -1;
    }
}

static void overlay_apply_display(void) {
    OverlayModeSel = overlay_find_mode(PresentScaleMode);
    OverlaySourceSel = overlay_find_source();
    OverlayScaleSel = overlay_find_scale_for_source(OverlaySourceSel, PresentScaleRatio);
}

static void overlay_mark_display_dirty(void) {
    RefreshBoundsValid = 0;
}

static void overlay_set_mode_index(int index) {
    if (index < 0) index = OVERLAY_MODE_COUNT - 1;
    if (index >= OVERLAY_MODE_COUNT) index = 0;
    OverlayModeSel = index;
    PresentScaleMode = OverlayModes[index].mode;
    overlay_mark_display_dirty();
}

static void overlay_set_scale_index(int index) {
    int count;
    const OverlayScalePreset *list = overlay_scale_list_for_source(OverlaySourceSel, &count);
    if (index < 0) index = count - 1;
    if (index >= count) index = 0;
    OverlayScaleSel = index;
    PresentScaleRatio = list[index].ratio;
    overlay_mark_display_dirty();
}

static void overlay_set_source_index(int index) {
    int ratio = overlay_current_scale()->ratio;
    if (index < 0) index = OVERLAY_SOURCE_COUNT - 1;
    if (index >= OVERLAY_SOURCE_COUNT) index = 0;
    OverlaySourceSel = index;
    PresentSourceFull = OverlaySources[index].full;
    PresentSourceWidth = OverlaySources[index].width;
    PresentSourceHeight = OverlaySources[index].height;
    ratio = overlay_preferred_scale_ratio_for_source(OverlaySourceSel, ratio);
    OverlayScaleSel = overlay_find_scale_for_source(OverlaySourceSel, ratio);
    PresentScaleRatio = overlay_current_scale()->ratio;
    overlay_mark_display_dirty();
}

static void overlay_set_profile_index(int index) {
    int count = PhoneMEInputGetProfileCount();
    if (count <= 0) {
        OverlayProfileSel = 0;
        return;
    }
    if (index < 0) index = count - 1;
    if (index >= count) index = 0;
    OverlayProfileSel = index;
    PhoneMEInputApplyProfile(index);
}

static void overlay_set_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    OverlayVolume = volume;
    MediaSDL_SetMasterVolume(OverlayVolume);
}

static void overlay_set_synth_index(int index) {
    if (index < 0) index = OVERLAY_SYNTH_COUNT - 1;
    if (index >= OVERLAY_SYNTH_COUNT) index = 0;
    OverlaySynthSel = index;
    MediaSDL_SetSynth(OverlaySynths[index].engine);
}

static int overlay_find_synth(const char *id) {
    int i;
    for (i = 0; id != NULL && i < OVERLAY_SYNTH_COUNT; i++) {
        if (strcmp(OverlaySynths[i].id, id) == 0) return i;
    }
    if (id != NULL && strcmp(id, "wt") == 0) return 1;
    return 0;
}

static void overlay_set_fps_index(int index) {
    if (index < 0) index = OVERLAY_FPS_LIMIT_COUNT - 1;
    if (index >= OVERLAY_FPS_LIMIT_COUNT) index = 0;
    OverlayFpsSel = index;
    FrameLimitFps = OverlayFpsLimits[index].fps;
    FrameLimitNext = 0;
}

static void overlay_save_config(void) {
    FILE *f;
    int i;

    if (OverlayConfigPath == NULL || OverlayConfigPath[0] == '\0') {
        return;
    }
    f = fopen(OverlayConfigPath, "w");
    if (f == NULL) {
        return;
    }

    fprintf(f, "SCALE=%s\n", overlay_current_scale()->id);
    fprintf(f, "VIEW=%s\n", OverlaySources[OverlaySourceSel].id);
    fprintf(f, "DISPLAY_MODE=%s\n", OverlayModes[OverlayModeSel].id);
    {
        int i;
        for (i = 0; i < OVERLAY_FILTER_COUNT; i++) {
            if (OverlayFilters[i].filter == PresentFilter) fprintf(f, "FILTER=%s\n", OverlayFilters[i].id);
        }
    }
    fprintf(f, "PAN_X=%d\n", PresentPanX);
    fprintf(f, "PAN_Y=%d\n", PresentPanY);
    fprintf(f, "FPS_LIMIT=%s\n", OverlayFpsLimits[OverlayFpsSel].id);
    fprintf(f, "VOLUME=%d\n", OverlayVolume);
    fprintf(f, "SYNTH=%s\n", OverlaySynths[OverlaySynthSel].id);
    fprintf(f, "PROFILE=%s\n", PhoneMEInputGetProfileId(OverlayProfileSel));
    for (i = 0; i < OVERLAY_BIND_COUNT; i++) {
        fprintf(f, "%s=%d\n", OverlayBindNames[i], PhoneMEInputGetBind(i));
    }
    fclose(f);
}

static void overlay_load_debug(void) {
    FILE *f;
    char line[128];
    int i;
    for (i = 0; i < OVERLAY_DEBUG_COUNT; i++) OverlayDebug[i] = OverlayDebugOptions[i].def;
    if (OverlayDebugPath == NULL || OverlayDebugPath[0] == '\0') return;
    f = fopen(OverlayDebugPath, "r");
    if (f == NULL) return;
    while (fgets(line, sizeof(line), f) != NULL) {
        char *eq = strchr(line, '=');
        if (eq == NULL) continue;
        *eq = '\0';
        for (i = 0; i < OVERLAY_DEBUG_COUNT; i++) {
            if (strcmp(line, OverlayDebugOptions[i].key) == 0) OverlayDebug[i] = atoi(eq + 1) != 0;
        }
    }
    fclose(f);
}

static void overlay_save_debug(void) {
    FILE *f;
    int i;
    if (OverlayDebugPath == NULL || OverlayDebugPath[0] == '\0') return;
    f = fopen(OverlayDebugPath, "w");
    if (f == NULL) return;
    for (i = 0; i < OVERLAY_DEBUG_COUNT; i++) {
        fprintf(f, "%s=%d\n", OverlayDebugOptions[i].key, OverlayDebug[i]);
    }
    fclose(f);
}

static int overlay_debug_needs_restart(void) {
    int i;
    for (i = 0; i < OVERLAY_DEBUG_COUNT; i++) {
        if (!OverlayDebugOptions[i].live && OverlayDebug[i] != OverlayDebugAtStart[i]) return 1;
    }
    return 0;
}

/* Restart: leave a flag for the launcher, then exit like "Exit" does */
static int overlay_can_restart(void) {
    const char *flag = getenv("PHONEME_RESTART_FLAG");
    return flag != NULL && flag[0] != '\0';
}

static void overlay_request_restart(void) {
    const char *flag = getenv("PHONEME_RESTART_FLAG");
    if (flag != NULL && flag[0] != '\0') {
        FILE *f = fopen(flag, "w");
        if (f != NULL) fclose(f);
    }
    overlay_save_config();
    OverlayVisible = 0;
    OverlayShutdownRequested = 1;
}

static void fps_count_frame(void) {
    Uint32 now;
    if (!ShowFps) return;
    now = SDL_GetTicks();
    if (FpsWindowStart == 0) FpsWindowStart = now;
    FpsFrames++;
    if (now - FpsWindowStart >= 1000) {
        FpsValue = (int) ((FpsFrames * 1000 + (now - FpsWindowStart) / 2) / (now - FpsWindowStart));
        FpsFrames = 0;
        FpsWindowStart = now;
    }
}

static void fps_draw(void) {
    char text[16];
    int w;
    if (!ShowFps || OverlayVisible) return;
    if (FpsValue < 0) snprintf(text, sizeof(text), "-- fps");
    else snprintf(text, sizeof(text), "%d fps", FpsValue);
    w = (int) strlen(text) * 8;
    boxColor(Native_SDL_Screen, 0, 0, (Sint16) (w + 3), 11, 0x000000a0);
    stringColor(Native_SDL_Screen, 2, 2, text, 0x00ff88ff);
}

static void overlay_init(void) {
    char *volumeText;
    int i;

    OverlayDebugPath = getenv("PHONEME_DEBUG_CONFIG");
    overlay_load_debug();
    for (i = 0; i < OVERLAY_DEBUG_COUNT; i++) OverlayDebugAtStart[i] = OverlayDebug[i];
    ShowFps = getenv("PHONEME_SHOW_FPS") != NULL;

    OverlayConfigPath = getenv("PHONEME_CONFIG_PATH");
    configure_frame_limit();
    overlay_apply_display();
    OverlayResolutionChanged = 0;
    OverlayProfileSel = PhoneMEInputGetProfileIndex();
    volumeText = getenv("PHONEME_SOUND_VOLUME");
    if (volumeText != NULL && volumeText[0] != '\0') {
        OverlayVolume = atoi(volumeText);
    } else {
        OverlayVolume = MediaSDL_GetMasterVolume();
    }
    overlay_set_volume(OverlayVolume);
    /* the engine reads PHONEME_SYNTH itself at start; just mirror it */
    OverlaySynthSel = overlay_find_synth(getenv("PHONEME_SYNTH"));
}


static void compute_center_layout(PresentLayout *layout) {
    if (layout->srcW > SDL_FULLWIDTH) {
        layout->srcX = (layout->srcW - SDL_FULLWIDTH) / 2;
        layout->srcW = SDL_FULLWIDTH;
    }
    if (layout->srcH > SDL_FULLHEIGHT) {
        layout->srcY = (layout->srcH - SDL_FULLHEIGHT) / 2;
        layout->srcH = SDL_FULLHEIGHT;
    }
    layout->dstW = layout->srcW;
    layout->dstH = layout->srcH;
    layout->dstX = (SDL_FULLWIDTH - layout->dstW) / 2;
    layout->dstY = (SDL_FULLHEIGHT - layout->dstH) / 2;
    layout->name = "center";
}

static void compute_fit_layout(PresentLayout *layout) {
    int dstWidth, dstHeight;

    dstWidth = SDL_FULLWIDTH;
    dstHeight = (layout->srcH * SDL_FULLWIDTH + (layout->srcW / 2)) / layout->srcW;
    if (dstHeight > SDL_FULLHEIGHT) {
        dstHeight = SDL_FULLHEIGHT;
        dstWidth = (layout->srcW * SDL_FULLHEIGHT + (layout->srcH / 2)) / layout->srcH;
    }

    layout->dstW = dstWidth;
    layout->dstH = dstHeight;
    layout->dstX = (SDL_FULLWIDTH - dstWidth) / 2;
    layout->dstY = (SDL_FULLHEIGHT - dstHeight) / 2;
    layout->name = "fit";
}

static void compute_crop_layout(PresentLayout *layout) {
    if (layout->srcW * SDL_FULLHEIGHT > layout->srcH * SDL_FULLWIDTH) {
        int croppedWidth = (layout->srcH * SDL_FULLWIDTH) / SDL_FULLHEIGHT;
        layout->srcX = (layout->srcW - croppedWidth) / 2;
        layout->srcW = croppedWidth;
    } else if (layout->srcH * SDL_FULLWIDTH > layout->srcW * SDL_FULLHEIGHT) {
        int croppedHeight = (layout->srcW * SDL_FULLHEIGHT) / SDL_FULLWIDTH;
        layout->srcY = (layout->srcH - croppedHeight) / 2;
        layout->srcH = croppedHeight;
    }
    layout->dstX = 0;
    layout->dstY = 0;
    layout->dstW = SDL_FULLWIDTH;
    layout->dstH = SDL_FULLHEIGHT;
    layout->name = "crop";
}

static void compute_zoom_layout(PresentLayout *layout) {
    int baseSrcX = layout->srcX;
    int baseSrcY = layout->srcY;

    layout->areaX = layout->srcX;
    layout->areaY = layout->srcY;
    layout->areaW = layout->srcW;
    layout->areaH = layout->srcH;
    int scaledWidth = (layout->srcW * PresentScaleRatio + 500) / 1000;
    int scaledHeight = (layout->srcH * PresentScaleRatio + 500) / 1000;

    if (scaledWidth < 1) scaledWidth = 1;
    if (scaledHeight < 1) scaledHeight = 1;

    if (scaledWidth > SDL_FULLWIDTH) {
        int visibleSrcW = (layout->srcW * SDL_FULLWIDTH + (scaledWidth / 2)) / scaledWidth;
        if (visibleSrcW < 1) visibleSrcW = 1;
        if (visibleSrcW > layout->srcW) visibleSrcW = layout->srcW;
        layout->srcX = baseSrcX + (layout->srcW - visibleSrcW) / 2;
        layout->srcW = visibleSrcW;
        layout->dstX = 0;
        layout->dstW = SDL_FULLWIDTH;
    } else {
        layout->dstX = (SDL_FULLWIDTH - scaledWidth) / 2;
        layout->dstW = scaledWidth;
    }

    if (scaledHeight > SDL_FULLHEIGHT) {
        int visibleSrcH = (layout->srcH * SDL_FULLHEIGHT + (scaledHeight / 2)) / scaledHeight;
        if (visibleSrcH < 1) visibleSrcH = 1;
        if (visibleSrcH > layout->srcH) visibleSrcH = layout->srcH;
        layout->srcY = baseSrcY + (layout->srcH - visibleSrcH) / 2;
        layout->srcH = visibleSrcH;
        layout->dstY = 0;
        layout->dstH = SDL_FULLHEIGHT;
    } else {
        layout->dstY = (SDL_FULLHEIGHT - scaledHeight) / 2;
        layout->dstH = scaledHeight;
    }

    layout->name = "zoom";
}

static int detect_active_bounds(SDL_Surface *surface, int width, int height,
                                int *srcX, int *srcY, int *srcW, int *srcH) {
    unsigned short *pixels;
    int pitchPixels;
    unsigned short bg;
    int x, y;
    int minX, minY, maxX, maxY;

    if (surface == NULL || width <= 0 || height <= 0) {
        return 0;
    }

    pixels = (unsigned short *)surface->pixels;
    pitchPixels = surface->pitch / 2;
    bg = pixels[(height - 1) * pitchPixels + (width - 1)];
    minX = width;
    minY = height;
    maxX = -1;
    maxY = -1;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (pixels[y * pitchPixels + x] != bg) {
                if (x < minX) minX = x;
                if (y < minY) minY = y;
                if (x > maxX) maxX = x;
                if (y > maxY) maxY = y;
            }
        }
    }

    if (maxX < minX || maxY < minY) {
        return 0;
    }

    *srcX = minX;
    *srcY = minY;
    *srcW = maxX - minX + 1;
    *srcH = maxY - minY + 1;
    return 1;
}

static int apply_refresh_bounds(int visibleWidth, int visibleHeight,
                                int *srcX, int *srcY, int *srcW, int *srcH) {
    int x1, y1, x2, y2;
    int width, height;

    if (!RefreshBoundsValid) {
        return 0;
    }

    x1 = RefreshMinX;
    y1 = RefreshMinY;
    x2 = RefreshMaxX;
    y2 = RefreshMaxY;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > visibleWidth) x2 = visibleWidth;
    if (y2 > visibleHeight) y2 = visibleHeight;

    width = x2 - x1;
    height = y2 - y1;
    if (width < 32 || height < 32) {
        return 0;
    }
    if (width >= (visibleWidth * 95) / 100 &&
        height >= (visibleHeight * 95) / 100) {
        return 0;
    }

    *srcX = x1;
    *srcY = y1;
    *srcW = width;
    *srcH = height;
    return 1;
}

static void update_refresh_bounds(int x1, int y1, int x2, int y2) {
    int fullW = OriginalWidth > 0 ? OriginalWidth : SDL_FULLWIDTH;
    int fullH = OriginalHeight > 0 ? OriginalHeight : SDL_FULLHEIGHT;

    if (x2 <= x1 || y2 <= y1) {
        return;
    }
    if (x1 <= 0 && y1 <= 0 &&
        x2 >= (fullW * 95) / 100 &&
        y2 >= (fullH * 95) / 100) {
        return;
    }
    if (!RefreshBoundsValid) {
        RefreshMinX = x1;
        RefreshMinY = y1;
        RefreshMaxX = x2;
        RefreshMaxY = y2;
        RefreshBoundsValid = 1;
        return;
    }
    if (x1 < RefreshMinX) RefreshMinX = x1;
    if (y1 < RefreshMinY) RefreshMinY = y1;
    if (x2 > RefreshMaxX) RefreshMaxX = x2;
    if (y2 > RefreshMaxY) RefreshMaxY = y2;
}

static void choose_zoom_source_bounds(SDL_Surface *surface, int visibleWidth,
                                      int visibleHeight, PresentLayout *layout) {
    int activeX, activeY, activeW, activeH;
    int refreshX, refreshY, refreshW, refreshH;
    int haveActive;
    int haveRefresh;

    if (PresentSourceFull) {
        layout->srcX = 0;
        layout->srcY = 0;
        layout->srcW = visibleWidth;
        layout->srcH = visibleHeight;
        compute_zoom_layout(layout);
        return;
    }

    if (PresentSourceWidth > 0 && PresentSourceHeight > 0 &&
        PresentSourceWidth <= visibleWidth && PresentSourceHeight <= visibleHeight) {
        layout->srcX = 0;
        layout->srcY = 0;
        layout->srcW = PresentSourceWidth;
        layout->srcH = PresentSourceHeight;
        compute_zoom_layout(layout);
        return;
    }

    /* scanning the whole frame for the content box every refresh was the
     * most expensive part of presenting: do it twice a second at most */
    {
        static int cachedValid = -1, cachedX, cachedY, cachedW, cachedH;
        static int cachedVisW, cachedVisH;
        static Uint32 cachedTicks;
        Uint32 now = SDL_GetTicks();
        if (cachedValid < 0 || cachedVisW != visibleWidth || cachedVisH != visibleHeight ||
            now - cachedTicks >= 500) {
            cachedValid = detect_active_bounds(surface, visibleWidth, visibleHeight,
                                               &cachedX, &cachedY, &cachedW, &cachedH);
            cachedVisW = visibleWidth;
            cachedVisH = visibleHeight;
            cachedTicks = now;
        }
        haveActive = cachedValid;
        activeX = cachedX;
        activeY = cachedY;
        activeW = cachedW;
        activeH = cachedH;
    }
    haveRefresh = apply_refresh_bounds(visibleWidth, visibleHeight,
                                       &refreshX, &refreshY, &refreshW, &refreshH);

    if (haveActive &&
        (activeW < (visibleWidth * 95) / 100 ||
         activeH < (visibleHeight * 95) / 100)) {
        layout->srcX = activeX;
        layout->srcY = activeY;
        layout->srcW = activeW;
        layout->srcH = activeH;
        compute_zoom_layout(layout);
        return;
    }

    if (haveRefresh) {
        layout->srcX = refreshX;
        layout->srcY = refreshY;
        layout->srcW = refreshW;
        layout->srcH = refreshH;
        compute_zoom_layout(layout);
    }
}

static void compute_integer_layout(PresentLayout *layout) {
    int kx = SDL_FULLWIDTH / layout->srcW, ky = SDL_FULLHEIGHT / layout->srcH;
    int k = kx < ky ? kx : ky;
    if (k < 1) {
        compute_center_layout(layout);
        layout->name = "integer-1x";
        return;
    }
    layout->dstW = layout->srcW * k;
    layout->dstH = layout->srcH * k;
    layout->dstX = (SDL_FULLWIDTH - layout->dstW) / 2;
    layout->dstY = (SDL_FULLHEIGHT - layout->dstH) / 2;
    layout->name = "integer";
}

static void compute_present_layout(int visibleWidth, int visibleHeight,
                                   PresentLayout *layout) {
    layout->srcX = 0;
    layout->srcY = 0;
    layout->srcW = visibleWidth;
    layout->srcH = visibleHeight;
    layout->dstX = 0;
    layout->dstY = 0;
    layout->dstW = SDL_FULLWIDTH;
    layout->dstH = SDL_FULLHEIGHT;
    layout->areaX = 0;
    layout->areaY = 0;
    layout->areaW = visibleWidth;
    layout->areaH = visibleHeight;
    layout->name = "fill";

    if (PresentScaleMode == PRESENT_SCALE_FILL) {
        layout->name = "fill";
        return;
    }
    if (PresentScaleMode == PRESENT_SCALE_CENTER) {
        compute_center_layout(layout);
        return;
    }
    if (PresentScaleMode == PRESENT_SCALE_FIT) {
        compute_fit_layout(layout);
        return;
    }
    if (PresentScaleMode == PRESENT_SCALE_CROP) {
        compute_crop_layout(layout);
        return;
    }
    if (PresentScaleMode == PRESENT_SCALE_ZOOM) {
        compute_zoom_layout(layout);
        return;
    }
    if (PresentScaleMode == PRESENT_SCALE_INTEGER) {
        compute_integer_layout(layout);
        return;
    }

    /*
     * Auto, by rule instead of a list of sizes:
     *  - native size: 1:1
     *  - a whole multiple filling >= 90% of the screen: integer scale
     *  - filling the screen loses <= 16% of the long side: crop (pannable)
     *  - otherwise: fit with bars
     * e.g. 128x128 crop (=fit), 176x208 crop, 176x220 / 128x160 / 240x320 fit
     */
    if (visibleWidth == SDL_FULLWIDTH && visibleHeight == SDL_FULLHEIGHT) {
        layout->name = "native";
        return;
    }
    {
        int kx = SDL_FULLWIDTH / visibleWidth, ky = SDL_FULLHEIGHT / visibleHeight;
        int k = kx < ky ? kx : ky;
        /* fit and fill factors as 1/1000 */
        int fitX = SDL_FULLWIDTH * 1000 / visibleWidth, fitY = SDL_FULLHEIGHT * 1000 / visibleHeight;
        int fit = fitX < fitY ? fitX : fitY, fill = fitX > fitY ? fitX : fitY;
        if (k >= 2 && (visibleWidth * k * 10 >= SDL_FULLWIDTH * 9 ||
                       visibleHeight * k * 10 >= SDL_FULLHEIGHT * 9)) {
            compute_integer_layout(layout);
            layout->name = "auto-integer";
        } else if ((fill - fit) * 100 <= fill * 16) {
            compute_crop_layout(layout);
            layout->name = "auto-crop";
        } else {
            compute_fit_layout(layout);
            layout->name = "auto-fit";
        }
    }
}

/* moves the visible window inside its area: pan -1000 (left/top) .. 1000 */
static void apply_pan(PresentLayout *layout) {
    int slack;
    slack = layout->areaW - layout->srcW;
    PresentSlackX = slack > 0;
    if (slack > 0) {
        layout->srcX = layout->areaX + slack / 2 + (PresentPanX * slack) / 2000;
        if (layout->srcX < layout->areaX) layout->srcX = layout->areaX;
        if (layout->srcX > layout->areaX + slack) layout->srcX = layout->areaX + slack;
    }
    slack = layout->areaH - layout->srcH;
    PresentSlackY = slack > 0;
    if (slack > 0) {
        layout->srcY = layout->areaY + slack / 2 + (PresentPanY * slack) / 2000;
        if (layout->srcY < layout->areaY) layout->srcY = layout->areaY;
        if (layout->srcY > layout->areaY + slack) layout->srcY = layout->areaY + slack;
    }
}

static void overlay_draw_row(int y, const char *left, const char *right,
                             int selected) {
    char text[80];
    Uint32 color = selected ? 0x00ff88ff : 0xffffffff;

    if (selected) {
        boxColor(Native_SDL_Screen, 12, y - 2, SDL_FULLWIDTH - 12, y + 9,
                 0x284040dd);
    }
    if (right != NULL && right[0] != '\0') {
        snprintf(text, sizeof(text), "%-12s %s", left, right);
    } else {
        snprintf(text, sizeof(text), "%s", left);
    }
    stringColor(Native_SDL_Screen, 18, y, text, color);
}

static void overlay_assigned_binds_for_phone(int phoneCode, char *out, int outSize) {
    int i;
    int used = 0;
    out[0] = '\0';
    for (i = 0; i < OVERLAY_BIND_COUNT; i++) {
        if (PhoneMEInputGetBind(i) == phoneCode) {
            if (used > 0 && used < outSize - 1) {
                out[used++] = ',';
                out[used] = '\0';
            }
            if (used < outSize - 1) {
                snprintf(out + used, outSize - used, "%s", OverlayBindLabels[i]);
                used = strlen(out);
            }
        }
    }
    if (out[0] == '\0') {
        snprintf(out, outSize, "-");
    }
}

static void overlay_draw_main(void) {
    const char *items[OVERLAY_MAIN_COUNT] = {
        "Resume", "Display", "Controls", "Sound", "Debug", "Restart game", "Exit"
    };
    int i;
    for (i = 0; i < OVERLAY_MAIN_COUNT; i++) {
        overlay_draw_row(50 + i * 15, items[i], "", OverlayRow == i);
    }
    if (OverlayRow == OVERLAY_MAIN_RESTART && !overlay_can_restart()) {
        stringColor(Native_SDL_Screen, 18, 160, "Exits; start the game", 0x9aaab0ff);
        stringColor(Native_SDL_Screen, 18, 172, "again to apply changes", 0x9aaab0ff);
    } else if (OverlayRow == OVERLAY_MAIN_RESTART) {
        stringColor(Native_SDL_Screen, 18, 160, "Applies new size and", 0x9aaab0ff);
        stringColor(Native_SDL_Screen, 18, 172, "debug settings", 0x9aaab0ff);
    }
}

static void overlay_draw_debug(void) {
    int i;
    for (i = 0; i < OVERLAY_DEBUG_COUNT; i++) {
        const char *value = OverlayDebug[i] ? "On" : "Off";
        if (i != 0 && i != OVERLAY_DEBUG_FPS && OverlayDebug[i] && !OverlayDebug[0]) {
            value = "On (no log)";
        }
        overlay_draw_row(48 + i * 13, OverlayDebugOptions[i].label, value, OverlayRow == i);
    }
    overlay_draw_row(48 + OVERLAY_DEBUG_COUNT * 13,
                     overlay_can_restart() ? "Restart now" : "Exit now", "",
                     OverlayRow == OVERLAY_DEBUG_ROW_RESTART);
    if (OverlayDebugPath == NULL || OverlayDebugPath[0] == '\0') {
        stringColor(Native_SDL_Screen, 18, 172, "Start the game from the", 0xffd060ff);
        stringColor(Native_SDL_Screen, 18, 184, "launcher to save these", 0xffd060ff);
    } else if (overlay_debug_needs_restart()) {
        stringColor(Native_SDL_Screen, 18, 172, "Restart the game to", 0xffd060ff);
        stringColor(Native_SDL_Screen, 18, 184, "apply the log settings", 0xffd060ff);
    } else if (OverlayRow == OVERLAY_DEBUG_FPS) {
        stringColor(Native_SDL_Screen, 18, 172, "Shown right away", 0x9aaab0ff);
    } else if (OverlayRow < OVERLAY_DEBUG_COUNT) {
        stringColor(Native_SDL_Screen, 18, 172, "Read logs in launcher:", 0x9aaab0ff);
        stringColor(Native_SDL_Screen, 18, 184, "Y > View log", 0x9aaab0ff);
    }
}

static const char *overlay_filter_label(void) {
    int i;
    for (i = 0; i < OVERLAY_FILTER_COUNT; i++) {
        if (OverlayFilters[i].filter == PresentFilter) return OverlayFilters[i].label;
    }
    return OverlayFilters[0].label;
}

static void overlay_draw_display(void) {
    overlay_draw_row(54, "Screen", OverlayModes[OverlayModeSel].label, OverlayRow == 0);
    overlay_draw_row(70, "Zoom", overlay_current_scale()->label, OverlayRow == 1);
    overlay_draw_row(86, "Size", OverlaySources[OverlaySourceSel].label, OverlayRow == 2);
    overlay_draw_row(102, "Filter", overlay_filter_label(), OverlayRow == 3);
    overlay_draw_row(118, "FPS", OverlayFpsLimits[OverlayFpsSel].label, OverlayRow == 4);
    overlay_draw_row(134, "Adjust view", ">", OverlayRow == 5);
    if (OverlayResolutionChanged) {
        stringColor(Native_SDL_Screen, 18, 158, "New size applies after", 0xffd060ff);
        stringColor(Native_SDL_Screen, 18, 170, "Menu > Restart game", 0xffd060ff);
    } else if (OverlayRow == 1 && PresentScaleMode != PRESENT_SCALE_ZOOM) {
        stringColor(Native_SDL_Screen, 18, 158, "Zoom works in", 0x9aaab0ff);
        stringColor(Native_SDL_Screen, 18, 170, "Manual zoom mode", 0x9aaab0ff);
    } else if (OverlayRow == 5) {
        stringColor(Native_SDL_Screen, 18, 158, "Move the picture when", 0x9aaab0ff);
        stringColor(Native_SDL_Screen, 18, 170, "it is cropped (HUD cut)", 0x9aaab0ff);
    }
}

/* Adjust view: the frozen game full screen, a hint bar, nothing else */
static void overlay_draw_view(void) {
    char line[40];
    /* keep the hint away from the edge being looked at */
    int y0 = PresentPanY > 0 ? 0 : SDL_FULLHEIGHT - 44;
    boxColor(Native_SDL_Screen, 0, y0, SDL_FULLWIDTH - 1, SDL_FULLHEIGHT - 1, 0x000000b0);
    if (PresentSlackX || PresentSlackY) {
        snprintf(line, sizeof(line), "D-pad: move  X %+d%% Y %+d%%",
                 PresentPanX / 10, PresentPanY / 10);
        stringColor(Native_SDL_Screen, 6, y0 + 4, line, 0xffffffff);
    } else {
        stringColor(Native_SDL_Screen, 6, y0 + 4, "Whole picture fits:", 0xffd060ff);
        stringColor(Native_SDL_Screen, 6, y0 + 14, "nothing to move", 0xffd060ff);
    }
    if (PresentScaleMode == PRESENT_SCALE_ZOOM) {
        snprintf(line, sizeof(line), "L1/R1: zoom %s", overlay_current_scale()->label);
        stringColor(Native_SDL_Screen, 6, y0 + (PresentSlackX || PresentSlackY ? 14 : 24), line, 0x9aaab0ff);
    }
    stringColor(Native_SDL_Screen, 6, y0 + 34, "B: center  A: done", 0x7df9ffff);
}

static void overlay_draw_sound(void) {
    char value[32];
    snprintf(value, sizeof(value), "%d%%", OverlayVolume);
    overlay_draw_row(54, "Volume", value, OverlayRow == 0);
    overlay_draw_row(70, "MIDI synth", OverlaySynths[OverlaySynthSel].label, OverlayRow == 1);
}

static void overlay_draw_controls(void) {
    int first;
    int i;
    int y;
    char value[80];
    int rowCount = OVERLAY_PHONE_COUNT + OVERLAY_CONTROL_ROW_OFFSET;

    if (OverlayRow < 8) {
        first = 0;
    } else {
        first = OverlayRow - 7;
    }
    if (first > rowCount - 10) {
        first = rowCount - 10;
    }
    if (first < 0) first = 0;

    for (i = first, y = 46; i < rowCount && y < SDL_FULLHEIGHT - 24; i++, y += 13) {
        if (i == 0) {
            overlay_draw_row(y, "PROFILE", PhoneMEInputGetProfileLabel(OverlayProfileSel),
                             OverlayRow == i);
        } else {
            int phoneIndex = i - OVERLAY_CONTROL_ROW_OFFSET;
            overlay_assigned_binds_for_phone(OverlayPhoneKeys[phoneIndex].code,
                                             value, sizeof(value));
            overlay_draw_row(y, OverlayPhoneKeys[phoneIndex].label, value, OverlayRow == i);
        }
    }
    if (OverlayCapture) {
        boxColor(Native_SDL_Screen, 18, SDL_FULLHEIGHT - 34, SDL_FULLWIDTH - 18,
                 SDL_FULLHEIGHT - 18, 0x303030ee);
        stringColor(Native_SDL_Screen, 24, SDL_FULLHEIGHT - 29,
                    "Press FunKey button", 0xffd060ff);
    }
}

static void overlay_draw(void) {
    if (!OverlayVisible) return;
    if (OverlayPage == OVERLAY_PAGE_VIEW) {
        overlay_draw_view();
        return;
    }

    /* the frozen game stays visible under a dimmed, translucent panel */
    boxColor(Native_SDL_Screen, 0, 0, SDL_FULLWIDTH - 1, SDL_FULLHEIGHT - 1,
             0x00000070);
    boxColor(Native_SDL_Screen, 8, 24, SDL_FULLWIDTH - 8, SDL_FULLHEIGHT - 12,
             0x101820b0);
    rectangleColor(Native_SDL_Screen, 8, 24, SDL_FULLWIDTH - 8, SDL_FULLHEIGHT - 12,
                   0x7df9ffff);

    if (OverlayPage == OVERLAY_PAGE_MAIN) {
        stringColor(Native_SDL_Screen, 18, 34, "phoneME menu", 0x7df9ffff);
        overlay_draw_main();
    } else if (OverlayPage == OVERLAY_PAGE_DISPLAY) {
        stringColor(Native_SDL_Screen, 18, 34, "Display", 0x7df9ffff);
        overlay_draw_display();
    } else if (OverlayPage == OVERLAY_PAGE_SOUND) {
        stringColor(Native_SDL_Screen, 18, 34, "Sound", 0x7df9ffff);
        overlay_draw_sound();
    } else if (OverlayPage == OVERLAY_PAGE_CONTROLS) {
        stringColor(Native_SDL_Screen, 18, 34, "Controls", 0x7df9ffff);
        overlay_draw_controls();
    } else if (OverlayPage == OVERLAY_PAGE_DEBUG) {
        stringColor(Native_SDL_Screen, 18, 34, "Debug (all games)", 0x7df9ffff);
        overlay_draw_debug();
    }

    stringColor(Native_SDL_Screen, 18, SDL_FULLHEIGHT - 20,
                "A:OK  B:Back  Power:Menu", 0x9aaab0ff);
}

static void overlay_adjust_selected(int direction) {
    if (OverlayPage == OVERLAY_PAGE_DISPLAY) {
        if (OverlayRow == 0) overlay_set_mode_index(OverlayModeSel + direction);
        else if (OverlayRow == 1) overlay_set_scale_index(OverlayScaleSel + direction);
        else if (OverlayRow == 2) {
            overlay_set_source_index(OverlaySourceSel + direction);
            OverlayResolutionChanged = 1;
        }
        else if (OverlayRow == 3) {
            int i, cur = 0;
            for (i = 0; i < OVERLAY_FILTER_COUNT; i++) if (OverlayFilters[i].filter == PresentFilter) cur = i;
            cur = (cur + direction + OVERLAY_FILTER_COUNT) % OVERLAY_FILTER_COUNT;
            PresentFilter = OverlayFilters[cur].filter;
        }
        else if (OverlayRow == 4) overlay_set_fps_index(OverlayFpsSel + direction);
        else if (OverlayRow == 5) {
            OverlayPage = OVERLAY_PAGE_VIEW;
            return;
        }
        overlay_save_config();
    } else if (OverlayPage == OVERLAY_PAGE_SOUND) {
        if (OverlayRow == 0) overlay_set_volume(OverlayVolume + direction * 5);
        else if (OverlayRow == 1) overlay_set_synth_index(OverlaySynthSel + direction);
        overlay_save_config();
    } else if (OverlayPage == OVERLAY_PAGE_CONTROLS) {
        if (OverlayRow == 0) {
            overlay_set_profile_index(OverlayProfileSel + direction);
            overlay_save_config();
        }
    } else if (OverlayPage == OVERLAY_PAGE_DEBUG) {
        if (OverlayRow < OVERLAY_DEBUG_COUNT) {
            OverlayDebug[OverlayRow] = !OverlayDebug[OverlayRow];
            if (OverlayRow == OVERLAY_DEBUG_FPS) {
                ShowFps = OverlayDebug[OverlayRow];
                FpsValue = -1;
                FpsFrames = 0;
                FpsWindowStart = 0;
            }
            overlay_save_debug();
        }
    }
}

static int overlay_row_count(void) {
    if (OverlayPage == OVERLAY_PAGE_MAIN) return OVERLAY_MAIN_COUNT;
    if (OverlayPage == OVERLAY_PAGE_DEBUG) return OVERLAY_DEBUG_COUNT + 1;
    if (OverlayPage == OVERLAY_PAGE_DISPLAY) return 6;
    if (OverlayPage == OVERLAY_PAGE_SOUND) return 2;
    if (OverlayPage == OVERLAY_PAGE_CONTROLS) return OVERLAY_PHONE_COUNT + OVERLAY_CONTROL_ROW_OFFSET;
    return 1;
}

static void overlay_back(void) {
    if (OverlayCapture) {
        OverlayCapture = 0;
        return;
    }
    if (OverlayPage == OVERLAY_PAGE_MAIN) {
        OverlayVisible = 0;
        overlay_save_config();
        return;
    }
    if (OverlayPage == OVERLAY_PAGE_VIEW) {
        overlay_save_config();
        OverlayPage = OVERLAY_PAGE_DISPLAY;
        OverlayRow = 5;
        return;
    }
    OverlayRow = OverlayPage == OVERLAY_PAGE_DISPLAY ? OVERLAY_MAIN_DISPLAY
               : OverlayPage == OVERLAY_PAGE_CONTROLS ? OVERLAY_MAIN_CONTROLS
               : OverlayPage == OVERLAY_PAGE_SOUND ? OVERLAY_MAIN_SOUND
               : OVERLAY_MAIN_DEBUG;
    OverlayPage = OVERLAY_PAGE_MAIN;
}

static void overlay_accept(void) {
    if (OverlayPage == OVERLAY_PAGE_MAIN) {
        if (OverlayRow == OVERLAY_MAIN_RESUME) {
            OverlayVisible = 0;
            overlay_save_config();
        } else if (OverlayRow == OVERLAY_MAIN_DISPLAY) {
            OverlayPage = OVERLAY_PAGE_DISPLAY;
            OverlayRow = 0;
        } else if (OverlayRow == OVERLAY_MAIN_CONTROLS) {
            OverlayPage = OVERLAY_PAGE_CONTROLS;
            OverlayRow = 0;
        } else if (OverlayRow == OVERLAY_MAIN_SOUND) {
            OverlayPage = OVERLAY_PAGE_SOUND;
            OverlayRow = 0;
        } else if (OverlayRow == OVERLAY_MAIN_DEBUG) {
            OverlayPage = OVERLAY_PAGE_DEBUG;
            OverlayRow = 0;
        } else if (OverlayRow == OVERLAY_MAIN_RESTART) {
            overlay_request_restart();
        } else if (OverlayRow == OVERLAY_MAIN_EXIT) {
            overlay_save_config();
            OverlayVisible = 0;
            OverlayShutdownRequested = 1;
        }
    } else if (OverlayPage == OVERLAY_PAGE_DEBUG &&
               OverlayRow == OVERLAY_DEBUG_ROW_RESTART) {
        overlay_request_restart();
    } else if (OverlayPage == OVERLAY_PAGE_CONTROLS) {
        if (OverlayRow == 0) {
            overlay_adjust_selected(1);
        } else {
            OverlayCapture = 1;
        }
    } else {
        overlay_adjust_selected(1);
    }
}

int PhoneMEOverlayHandleSDLEvent(SDL_Event *event) {
    SDLKey key;
    int rows;
    int bindIndex;

    if (event->type != SDL_KEYDOWN && event->type != SDL_KEYUP) {
        return 0;
    }
    key = event->key.keysym.sym;

    if (!OverlayVisible) {
        if (event->type == SDL_KEYDOWN && (key == SDLK_q || key == SDLK_ESCAPE)) {
            OverlayVisible = 1;
            OverlayPage = OVERLAY_PAGE_MAIN;
            OverlayRow = 0;
            OverlayCapture = 0;
            overlay_apply_display();
            OverlayProfileSel = PhoneMEInputGetProfileIndex();
            OverlayVolume = MediaSDL_GetMasterVolume();
            return 1;
        }
        return 0;
    }

    if (event->type == SDL_KEYUP) {
        return 1;
    }

    if (OverlayCapture) {
        if (key == SDLK_q || key == SDLK_ESCAPE) {
            OverlayCapture = 0;
            return 1;
        }
        bindIndex = overlay_physical_bind_index(key);
        if (bindIndex >= 0 && OverlayPage == OVERLAY_PAGE_CONTROLS &&
            OverlayRow >= OVERLAY_CONTROL_ROW_OFFSET &&
            OverlayRow < OVERLAY_PHONE_COUNT + OVERLAY_CONTROL_ROW_OFFSET) {
            int phoneIndex = OverlayRow - OVERLAY_CONTROL_ROW_OFFSET;
            PhoneMEInputSetBind(bindIndex, OverlayPhoneKeys[phoneIndex].code);
            overlay_save_config();
        }
        OverlayCapture = 0;
        return 1;
    }

    if (OverlayPage == OVERLAY_PAGE_VIEW) {
        const int step = 100;   /* 10 presses from the center to an edge */
        if (key == SDLK_u || key == SDLK_UP) PresentPanY -= step;
        else if (key == SDLK_d || key == SDLK_DOWN) PresentPanY += step;
        else if (key == SDLK_l || key == SDLK_LEFT) PresentPanX -= step;
        else if (key == SDLK_r || key == SDLK_RIGHT) PresentPanX += step;
        else if (key == SDLK_b) PresentPanX = PresentPanY = 0;
        else if ((key == SDLK_n || key == SDLK_m) && PresentScaleMode == PRESENT_SCALE_ZOOM)
            overlay_set_scale_index(OverlayScaleSel + (key == SDLK_m ? 1 : -1));
        else if (key == SDLK_a || key == SDLK_RETURN || key == SDLK_SPACE ||
                 key == SDLK_q || key == SDLK_ESCAPE)
            overlay_back();
        if (PresentPanX < -1000) PresentPanX = -1000;
        if (PresentPanX > 1000) PresentPanX = 1000;
        if (PresentPanY < -1000) PresentPanY = -1000;
        if (PresentPanY > 1000) PresentPanY = 1000;
        return 1;
    }
    if (key == SDLK_q || key == SDLK_ESCAPE) {
        overlay_back();
        return 1;
    }
    if (key == SDLK_b) {
        overlay_back();
        return 1;
    }
    if (key == SDLK_a || key == SDLK_RETURN || key == SDLK_SPACE) {
        overlay_accept();
        return 1;
    }
    if (key == SDLK_u || key == SDLK_UP) {
        rows = overlay_row_count();
        OverlayRow--;
        if (OverlayRow < 0) OverlayRow = rows - 1;
        return 1;
    }
    if (key == SDLK_d || key == SDLK_DOWN) {
        rows = overlay_row_count();
        OverlayRow++;
        if (OverlayRow >= rows) OverlayRow = 0;
        return 1;
    }
    if (key == SDLK_l || key == SDLK_LEFT) {
        overlay_adjust_selected(-1);
        return 1;
    }
    if (key == SDLK_r || key == SDLK_RIGHT) {
        overlay_adjust_selected(1);
        return 1;
    }
    return 1;
}

void PhoneMEOverlayRefresh(void);

int PhoneMEOverlayIsVisible(void) {
    return OverlayVisible;
}

/*
 * The menu is modal: while it is open this loop owns the event queue, so the
 * VM (and every Java thread of the game) is frozen until it closes. Audio is
 * paused at the mixer: notes neither advance nor get stuck, and resume where
 * they were.
 */
void PhoneMEOverlayRunModal(void) {
    SDL_Event ev;
    MediaSDL_PauseAll(1);
    PhoneMEOverlayRefresh();
    while (OverlayVisible) {
        int redraw = 0;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                OverlayVisible = 0;
                OverlayShutdownRequested = 1;
                break;
            }
            if (PhoneMEOverlayHandleSDLEvent(&ev)) redraw = 1;
            if (!OverlayVisible) break;
        }
        if (redraw) PhoneMEOverlayRefresh();
        if (OverlayVisible) SDL_Delay(16);
    }
    MediaSDL_PauseAll(0);
    FrameLimitNext = 0;
    PhoneMEOverlayRefresh();
}

int PhoneMEOverlayConsumeShutdown(void) {
    if (!OverlayShutdownRequested) {
        return 0;
    }
    OverlayShutdownRequested = 0;
    return 1;
}

/* ─────────────────────────────────────────────────────
 * Game error screen.
 *
 * A game thread that dies with an exception leaves the screen frozen
 * (black, white or the last frame) and the keys dead. The VM reports every
 * uncaught exception here. The game counts as dead when no new frame follows:
 *  - an Error (OutOfMemoryError, NoClassDefFoundError...): within 3 s;
 *  - an exception: a key pressed afterwards gets no new frame within 1.5 s
 *    (a static menu whose helper thread died still answers keys).
 * Then this screen says so: A restarts, B exits.
 * A game that has not drawn a single frame after 60 s gets a softer notice
 * (A: keep waiting, B: exit).
 * ──────────────────────────────────────────────────── */
static char FatalClass[96];
static char FatalMsg[128];
static volatile Uint32 FatalTicks;      /* when a thread died, 0 = none pending */
static Uint32 FatalFrameTicks;          /* last game frame */
static Uint32 FatalStartTicks;
static int FatalVisible;                /* 1 = error, 2 = nothing drawn yet */
static int FatalNoticeDone;
static Uint32 FatalExitTicks;
static int FatalIsError;                /* the class name ends with "Error" */
static Uint32 FatalKeyTicks;            /* first key after the exception */

void PhoneMEUncaughtException(const char *cls, const char *msg) {
    if (FatalTicks != 0 || FatalVisible) return;   /* keep the first one */
    snprintf(FatalClass, sizeof(FatalClass), "%s", cls ? cls : "");
    snprintf(FatalMsg, sizeof(FatalMsg), "%s", msg ? msg : "");
    {
        size_t n = strlen(FatalClass);
        FatalIsError = n > 5 && strcmp(FatalClass + n - 5, "Error") == 0;
    }
    FatalKeyTicks = 0;
    FatalTicks = SDL_GetTicks();
    if (FatalTicks == 0) FatalTicks = 1;
}

/* a key went to the game */
void PhoneMEFatalNoteKey(void) {
    if (FatalTicks != 0 && FatalKeyTicks == 0) {
        FatalKeyTicks = SDL_GetTicks();
        if (FatalKeyTicks == 0) FatalKeyTicks = 1;
    }
}

static void fatal_frame_tick(void) {
    FatalFrameTicks = SDL_GetTicks();
}

/* draws a long line in pieces that fit the panel */
static int fatal_draw_wrapped(int y, const char *text, Uint32 color) {
    char line[40];
    int cols = (SDL_FULLWIDTH - 36) / 8;
    int len = (int)strlen(text);
    int pos = 0;
    if (cols > (int)sizeof(line) - 1) cols = sizeof(line) - 1;
    while (pos < len && y < SDL_FULLHEIGHT - 44) {
        int n = len - pos;
        if (n > cols) {
            int k = cols;               /* break at the last space that fits */
            while (k > 0 && text[pos + k] != ' ') k--;
            n = k > 0 ? k : cols;
        }
        memcpy(line, text + pos, n);
        line[n] = 0;
        stringColor(Native_SDL_Screen, 18, y, line, color);
        pos += n;
        while (pos < len && text[pos] == ' ') pos++;
        y += 11;
    }
    return y;
}

/* "  0" (a bytecode index) is no message */
static int fatal_has_text(const char *s) {
    for (; *s; s++) {
        if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')) return 1;
    }
    return 0;
}

static void fatal_draw(void) {
    int y;
    const char *name;
    if (!FatalVisible) return;
    boxColor(Native_SDL_Screen, 0, 0, SDL_FULLWIDTH - 1, SDL_FULLHEIGHT - 1, 0x000000a0);
    boxColor(Native_SDL_Screen, 8, 24, SDL_FULLWIDTH - 8, SDL_FULLHEIGHT - 12, 0x301010e0);
    rectangleColor(Native_SDL_Screen, 8, 24, SDL_FULLWIDTH - 8, SDL_FULLHEIGHT - 12, 0xff6060ff);
    y = 34;
    if (FatalVisible == 1) {
        stringColor(Native_SDL_Screen, 18, y, "Game error", 0xff6060ff);
        y += 18;
        name = strrchr(FatalClass, '/');
        name = name ? name + 1 : FatalClass;
        y = fatal_draw_wrapped(y, name, 0xffffffff);
        if (fatal_has_text(FatalMsg)) y = fatal_draw_wrapped(y, FatalMsg, 0xd0d0d0ff);
        y += 6;
        y = fatal_draw_wrapped(y, "The game stopped and will not continue.", 0x9aaab0ff);
        y = fatal_draw_wrapped(y, "Details: Debug > View log", 0x9aaab0ff);
        stringColor(Native_SDL_Screen, 18, SDL_FULLHEIGHT - 32,
                    overlay_can_restart() ? "A:Restart  B:Exit" : "A/B:Exit", 0xffd060ff);
    } else {
        stringColor(Native_SDL_Screen, 18, y, "No picture", 0xffd060ff);
        y += 18;
        y = fatal_draw_wrapped(y, "The game has not drawn anything for a minute.", 0xffffffff);
        y = fatal_draw_wrapped(y, "It may be stuck or unsupported.", 0xffffffff);
        y += 6;
        y = fatal_draw_wrapped(y, "Details: Debug > View log", 0x9aaab0ff);
        stringColor(Native_SDL_Screen, 18, SDL_FULLHEIGHT - 32, "A:Wait  B:Exit", 0xffd060ff);
    }
}

/*
 * Called from the VM's signal check (also while every Java thread is
 * blocked). Returns 1 when the game should be shut down.
 */
int PhoneMEFatalPoll(void) {
    Uint32 now = SDL_GetTicks();
    SDL_Event ev;
    int shutdown = 0;

    if (FatalExitTicks != 0) {
        /* the graceful shutdown did not get through (VM out of memory...) */
        if (now - FatalExitTicks > 6000) {
            fprintf(stderr, "phoneME: game error screen: forced exit\n");
            fflush(stderr);
            sync();
            _exit(1);
        }
        return 0;
    }
    if (FatalStartTicks == 0) FatalStartTicks = now;
    if (OverlayVisible) return 0;
    if (FatalTicks != 0) {
        if (FatalFrameTicks > FatalTicks + 500) {
            FatalTicks = 0;               /* still drawing: the game lives on */
        } else if (FatalIsError && now - FatalTicks > 3000) {
            FatalVisible = 1;
        } else if (FatalKeyTicks != 0 && now - FatalTicks > 3000 &&
                   FatalFrameTicks < FatalKeyTicks && now - FatalKeyTicks > 1500) {
            FatalVisible = 1;
        }
    }
    if (!FatalVisible && !FatalNoticeDone && FatalFrameTicks == 0 &&
        now - FatalStartTicks > 60000) {
        FatalVisible = 2;
    }
    if (!FatalVisible) return 0;

    fprintf(stderr, "phoneME: %s screen (%s %s)\n",
            FatalVisible == 1 ? "game error" : "no picture", FatalClass, FatalMsg);
    fflush(stderr);
    MediaSDL_PauseAll(1);
    PhoneMEOverlayRefresh();
    while (FatalVisible) {
        while (SDL_PollEvent(&ev)) {
            SDLKey key;
            if (ev.type == SDL_QUIT) {
                FatalVisible = 0;
                shutdown = 1;
                break;
            }
            if (ev.type != SDL_KEYDOWN) continue;
            key = ev.key.keysym.sym;
            if (key == SDLK_a || key == SDLK_RETURN || key == SDLK_SPACE) {
                if (FatalVisible == 2) {
                    FatalVisible = 0;
                    FatalNoticeDone = 1;
                } else {
                    FatalVisible = 0;
                    if (overlay_can_restart()) {
                        overlay_request_restart();
                        OverlayShutdownRequested = 0;
                    }
                    shutdown = 1;
                }
            } else if (key == SDLK_b || key == SDLK_q || key == SDLK_ESCAPE ||
                       key == SDLK_x || key == SDLK_s) {
                FatalVisible = 0;
                shutdown = 1;
            }
            if (!FatalVisible) break;
        }
        if (FatalVisible) SDL_Delay(30);
    }
    if (shutdown) {
        overlay_save_config();
        FatalExitTicks = SDL_GetTicks();
        return 1;
    }
    MediaSDL_PauseAll(0);
    FrameLimitNext = 0;
    PhoneMEOverlayRefresh();
    return 0;
}

static void rotate_surface_90(unsigned short *srcPixels, int srcPitchPixels,
                              int srcWidth, int srcHeight,
                              unsigned short *dstPixels, int dstPitchPixels) {
    int x, y;

    for (y = 0; y < srcHeight; y++) {
        for (x = 0; x < srcWidth; x++) {
            dstPixels[x * dstPitchPixels + (srcHeight - 1 - y)] =
                srcPixels[y * srcPitchPixels + x];
        }
    }
}

static void present_surface(SDL_Surface *sourceSurface, int rotateSource) {
    SDL_Surface *visibleSurface;
    unsigned short *srcPixels;
    unsigned short *dstPixels;
    int srcPitchPixels;
    int dstPitchPixels;
    int visibleWidth, visibleHeight;
    PresentLayout layout;
    static int lastSrcW = -1, lastSrcH = -1;
    static int lastDstX = -1, lastDstY = -1, lastDstW = -1, lastDstH = -1;
    static const char *lastName = NULL;

    visibleSurface = sourceSurface;
    if (rotateSource) {
        SDL_LockSurface(Native_SDL_PScreen);
        rotate_surface_90((unsigned short *)sourceSurface->pixels,
                          sourceSurface->pitch / 2,
                          sourceSurface->w,
                          sourceSurface->h,
                          (unsigned short *)Native_SDL_PScreen->pixels,
                          Native_SDL_PScreen->pitch / 2);
        SDL_UnlockSurface(Native_SDL_PScreen);
        visibleSurface = Native_SDL_PScreen;
        visibleWidth = sourceSurface->h;
        visibleHeight = sourceSurface->w;
    } else {
        visibleWidth = sourceSurface->w;
        visibleHeight = sourceSurface->h;
    }

    compute_present_layout(visibleWidth, visibleHeight, &layout);
    if (PresentScaleMode == PRESENT_SCALE_ZOOM) {
        choose_zoom_source_bounds(visibleSurface, visibleWidth, visibleHeight,
                                  &layout);
    }
    apply_pan(&layout);
    if (DisplayDebug &&
        (lastSrcW != layout.srcW || lastSrcH != layout.srcH ||
        lastDstX != layout.dstX || lastDstY != layout.dstY ||
        lastDstW != layout.dstW || lastDstH != layout.dstH ||
        lastName != layout.name)) {
        fprintf(stderr,
                "lfjport_present: mode=%s layout=%s visible=%dx%d src=%d,%d %dx%d dst=%d,%d %dx%d\n",
                present_scale_mode_name(), layout.name, visibleWidth, visibleHeight,
                layout.srcX, layout.srcY, layout.srcW, layout.srcH,
                layout.dstX, layout.dstY, layout.dstW, layout.dstH);
        lastSrcW = layout.srcW;
        lastSrcH = layout.srcH;
        lastDstX = layout.dstX;
        lastDstY = layout.dstY;
        lastDstW = layout.dstW;
        lastDstH = layout.dstH;
        lastName = layout.name;
    }
    /* clear only the bars around the picture */
    {
        SDL_Rect r;
        if (layout.dstY > 0) {
            r.x = 0; r.y = 0; r.w = SDL_FULLWIDTH; r.h = (Uint16) layout.dstY;
            SDL_FillRect(Native_SDL_Screen, &r, 0);
        }
        if (layout.dstY + layout.dstH < SDL_FULLHEIGHT) {
            r.x = 0; r.y = (Sint16) (layout.dstY + layout.dstH);
            r.w = SDL_FULLWIDTH; r.h = (Uint16) (SDL_FULLHEIGHT - layout.dstY - layout.dstH);
            SDL_FillRect(Native_SDL_Screen, &r, 0);
        }
        if (layout.dstX > 0) {
            r.x = 0; r.y = (Sint16) layout.dstY; r.w = (Uint16) layout.dstX; r.h = (Uint16) layout.dstH;
            SDL_FillRect(Native_SDL_Screen, &r, 0);
        }
        if (layout.dstX + layout.dstW < SDL_FULLWIDTH) {
            r.x = (Sint16) (layout.dstX + layout.dstW); r.y = (Sint16) layout.dstY;
            r.w = (Uint16) (SDL_FULLWIDTH - layout.dstX - layout.dstW); r.h = (Uint16) layout.dstH;
            SDL_FillRect(Native_SDL_Screen, &r, 0);
        }
    }
    SDL_LockSurface(Native_SDL_Screen);

    srcPixels = (unsigned short *)visibleSurface->pixels;
    dstPixels = (unsigned short *)Native_SDL_Screen->pixels;
    srcPitchPixels = visibleSurface->pitch / 2;
    dstPitchPixels = Native_SDL_Screen->pitch / 2;

    /* tap tables are rebuilt only when the layout or the filter changes */
    if (scale_prepare(&PresentScaler, layout.srcX, layout.srcY, layout.srcW, layout.srcH,
                      layout.dstW, layout.dstH, PresentFilter)) {
        scale_run(&PresentScaler, srcPixels, srcPitchPixels,
                  dstPixels + layout.dstY * dstPitchPixels + layout.dstX, dstPitchPixels);
    }

    SDL_UnlockSurface(Native_SDL_Screen);
    fps_draw();
    overlay_draw();
    fatal_draw();
}

void PhoneMEOverlayRefresh(void) {
    if (Native_SDL_Screen == NULL ||
        Native_SDL_HScreen == NULL ||
        Native_SDL_VScreen == NULL) {
        return;
    }

    if (Native_SDL_ScreenOrientation) {
        present_surface(Native_SDL_VScreen, OriginalOrientation == 0);
    } else {
        present_surface(Native_SDL_HScreen, OriginalOrientation == 1);
    }
    SDL_Flip(Native_SDL_Screen);
}

/**
 *
 * Generic Screen Buffer required by putpixel library.
 *
 */
gxj_screen_buffer gxj_system_screen_buffer;

/**
 * @file
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 */

/**
 * Initializes the lfjport_ui_ native resources.
 *
 * @return <tt>0</tt> upon successful initialization, or
 *         <tt>other value</tt> otherwise
 */
int lfjport_ui_init() 
{ int audioStatus;
  DisplayDebug = getenv("PHONEME_DISPLAY_DEBUG") != NULL;
  if (DisplayDebug) {
    fprintf(stderr, "lfjport_ui_init: begin %dx%d\n",
            SDL_FULLWIDTH, SDL_FULLHEIGHT);
  }
  if (SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_VIDEO) != 0) 
   { fprintf(stderr, "lfjport_ui_init: SDL_Init failed: %s\n", SDL_GetError());
     return(-1);
   }
  if (DisplayDebug) fprintf(stderr, "lfjport_ui_init: SDL_Init ok\n");
  SDL_ShowCursor(SDL_DISABLE);
  if (getenv("PHONEME_ENABLE_AUDIO") != NULL)
   { audioStatus = InitAudioSubsystem();
     if (DisplayDebug) fprintf(stderr, "lfjport_ui_init: InitAudioSubsystem=%d\n", audioStatus);
   }
  else
   { if (DisplayDebug) fprintf(stderr, "lfjport_ui_init: audio disabled\n"); }
  if (DisplayDebug) fprintf(stderr, "lfjport_ui_init: joysticks=%d\n", SDL_NumJoysticks());
  if (SDL_NumJoysticks() > 0) {
    SDL_JoystickOpen(0);
    SDL_JoystickEventState(SDL_ENABLE);
  }
  OriginalOrientation = 0;
  if (getenv("J2ME_GP2X_REVERSE") != NULL) OriginalOrientation = 1;
  OriginalWidth = OriginalOrientation ? SDL_FULLHEIGHT : SDL_FULLWIDTH;
  OriginalHeight = OriginalOrientation ? SDL_FULLWIDTH : SDL_FULLHEIGHT;
  configure_logical_screen_size();
  configure_present_scale_mode();
  if (getenv("PHONEME_ENABLE_GP2X_KEYS") != NULL) {
    InitGP2XKeys();
    if (DisplayDebug) fprintf(stderr, "lfjport_ui_init: InitGP2XKeys ok\n");
  } else {
    if (DisplayDebug) fprintf(stderr, "lfjport_ui_init: GP2X keys disabled\n");
  }
  overlay_init();
  PhoneMEProfileInit();
  Native_SDL_Screen = SDL_SetVideoMode(SDL_FULLWIDTH, SDL_FULLHEIGHT, 16, 0);
  if (Native_SDL_Screen == NULL)
   { fprintf(stderr, "lfjport_ui_init: SDL_SetVideoMode failed: %s\n",
             SDL_GetError());
     return(-2);
   }
  if (DisplayDebug) fprintf(stderr, "lfjport_ui_init: SDL_SetVideoMode ok\n");
  SDL_FillRect(Native_SDL_Screen, NULL,
               SDL_MapRGB(Native_SDL_Screen->format, 0, 64, 96));
  SDL_Flip(Native_SDL_Screen);
  if (DisplayDebug) fprintf(stderr, "lfjport_ui_init: initial fill ok\n");
  Native_SDL_HScreen = SDL_CreateRGBSurface(SDL_SWSURFACE, OriginalWidth, OriginalHeight, 16, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000);
  if (Native_SDL_HScreen == NULL)
   { fprintf(stderr, "lfjport_ui_init: SDL_CreateRGBSurface H failed: %s\n",
             SDL_GetError());
     return(-3);
   }
  Native_SDL_VScreen = SDL_CreateRGBSurface(SDL_SWSURFACE, OriginalHeight, OriginalWidth, 16, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000);
  if (Native_SDL_VScreen == NULL)
   { fprintf(stderr, "lfjport_ui_init: SDL_CreateRGBSurface V failed: %s\n",
             SDL_GetError());
     return(-4);
   }
  Native_SDL_PScreen = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                            OriginalHeight > OriginalWidth ? OriginalHeight : OriginalWidth,
                                            OriginalHeight > OriginalWidth ? OriginalHeight : OriginalWidth,
                                            16, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000);
  if (Native_SDL_PScreen == NULL)
   { fprintf(stderr, "lfjport_ui_init: SDL_CreateRGBSurface P failed: %s\n",
             SDL_GetError());
     return(-5);
   }
  SDL_LockSurface(Native_SDL_HScreen);
  SDL_LockSurface(Native_SDL_VScreen);
  gxj_system_screen_buffer.width = OriginalWidth;
  gxj_system_screen_buffer.height = OriginalHeight;
  gxj_system_screen_buffer.alphaData = NULL;
  gxj_system_screen_buffer.pixelData = Native_SDL_HScreen->pixels;
  Native_SDL_ScreenOrientation = KNI_FALSE;
  Native_SDL_Fullscreen = KNI_FALSE;
  atexit(SDL_Quit);
  if (DisplayDebug) fprintf(stderr, "lfjport_ui_init: ok\n");
  fflush(stderr);
  return 0;
}

/**
 * Finalize the lfjport_ui_ native resources.
 */
void lfjport_ui_finalize() 
{ if (getenv("PHONEME_ENABLE_AUDIO") != NULL) FinalizeAudioSubsystem();
  SDL_Quit();
}

/**
 * Bridge function to request a repaint 
 * of the area specified.
 *
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */

void VideoCopyRotate(unsigned short *Buffer, unsigned short *Video)
{ int x, y, bufinc, rowinc;
  Buffer = &Buffer[(SDL_FULLWIDTH-1)*SDL_FULLHEIGHT];
  bufinc = SDL_FULLHEIGHT;
  rowinc = ((SDL_FULLWIDTH-1)*SDL_FULLHEIGHT)+1;
  for(y=0; y<SDL_FULLHEIGHT; y++, Buffer+=rowinc+bufinc)
     for(x=0; x<SDL_FULLWIDTH; x++, Video++, Buffer-=bufinc)
          *Video = *Buffer;
}

void lfjport_refresh(int x1, int y1, int x2, int y2)
{
  wait_frame_limit();
  fps_count_frame();
  fatal_frame_tick();
  update_refresh_bounds(x1, y1, x2, y2);
  SDL_UnlockSurface(Native_SDL_HScreen);
  SDL_UnlockSurface(Native_SDL_VScreen);
  if (Native_SDL_ScreenOrientation) {
      present_surface(Native_SDL_VScreen, OriginalOrientation == 0);
  } else {
      present_surface(Native_SDL_HScreen, OriginalOrientation == 1);
  }
  /* SDL_Flip alone pushes the frame (on a software surface it is the full
   * update); calling SDL_UpdateRect before it copied every frame twice */
  SDL_Flip(Native_SDL_Screen);
  SDL_LockSurface(Native_SDL_HScreen);
  SDL_LockSurface(Native_SDL_VScreen);
}

/**
 * Porting API function to update scroll bar.
 *
 * @param scrollPosition current scroll position
 * @param scrollProportion maximum scroll position
 * @return status of this call
 */
int lfjport_set_vertical_scroll(int scrollPosition, int scrollProportion)
{ (void)scrollPosition;
  (void)scrollProportion;
  return 0;
}


/**
 * Turn on or off the full screen mode
 *
 * @param mode true for full screen mode
 *             false for normal
 */
void lfjport_set_fullscreen_mode(jboolean mode) 
{ Native_SDL_Fullscreen = mode;
}

/**
 * Resets native resources when foreground is gained by a new display.
 */
void lfjport_gained_foreground() 
{ // SDL_Quit();
}

/**
 * Change screen orientation flag
 */
jboolean lfjport_reverse_orientation() 
{ Native_SDL_ScreenOrientation = !Native_SDL_ScreenOrientation;
  gxj_system_screen_buffer.pixelData = Native_SDL_ScreenOrientation ? Native_SDL_VScreen->pixels : Native_SDL_HScreen->pixels;
  gxj_system_screen_buffer.width = Native_SDL_ScreenOrientation ? OriginalHeight : OriginalWidth;
  gxj_system_screen_buffer.height = Native_SDL_ScreenOrientation ? OriginalWidth : OriginalHeight;
  return Native_SDL_ScreenOrientation;
}

/**
 * Change screen orientation flag
 */
jboolean lfjport_get_reverse_orientation() 
{ return Native_SDL_ScreenOrientation;        
}

/**
 * Return screen width
 */
int lfjport_get_screen_width() 
{ return Native_SDL_ScreenOrientation ? OriginalHeight : OriginalWidth;
}

/**
 * Return screen height
 */
int lfjport_get_screen_height() 
{ return Native_SDL_ScreenOrientation ? OriginalWidth : OriginalHeight;
}
