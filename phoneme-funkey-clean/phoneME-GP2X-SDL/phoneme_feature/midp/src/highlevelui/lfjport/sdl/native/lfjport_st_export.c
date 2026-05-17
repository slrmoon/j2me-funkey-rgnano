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
#include <lfjport_export.h>
#include <gxj_putpixel.h>
#include <gxj_screen_buffer.h>

#include "SDL.h"
#include "SDL_gfxPrimitives.h"
#include "midp_constants_data.h"

#define SDL_FULLWIDTH	FULLWIDTH
#define SDL_FULLHEIGHT	FULLHEIGHT
void InitGP2XKeys(void);
int  InitAudioSubsystem(void);
void FinalizeAudioSubsystem(void);
void MediaSDL_SetMasterVolume(int volume);
int  MediaSDL_GetMasterVolume(void);
int  PhoneMEInputGetBind(int index);
void PhoneMEInputSetBind(int index, int key);
int  PhoneMEInputGetProfileCount(void);
const char *PhoneMEInputGetProfileId(int index);
const char *PhoneMEInputGetProfileLabel(int index);
int  PhoneMEInputGetProfileIndex(void);
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
static int       DisplayDebug;
static int       RefreshBoundsValid;
static int       RefreshMinX, RefreshMinY, RefreshMaxX, RefreshMaxY;

enum {
    PRESENT_SCALE_AUTO = 0,
    PRESENT_SCALE_FILL,
    PRESENT_SCALE_CENTER,
    PRESENT_SCALE_FIT,
    PRESENT_SCALE_CROP,
    PRESENT_SCALE_ZOOM
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

static const OverlayModePreset OverlayModes[] = {
    { "auto", "Auto", PRESENT_SCALE_AUTO },
    { "fit", "Fit all", PRESENT_SCALE_FIT },
    { "fill", "Stretch", PRESENT_SCALE_FILL },
    { "crop", "Crop fill", PRESENT_SCALE_CROP },
    { "center", "Center", PRESENT_SCALE_CENTER },
    { "zoom", "Manual zoom", PRESENT_SCALE_ZOOM }
};
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
static int OverlayProfileSel;
static int OverlayVolume = 100;
static const char *OverlayConfigPath;

enum {
    OVERLAY_PAGE_MAIN = 0,
    OVERLAY_PAGE_DISPLAY,
    OVERLAY_PAGE_SOUND,
    OVERLAY_PAGE_CONTROLS
};

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
    fprintf(f, "VOLUME=%d\n", OverlayVolume);
    fprintf(f, "PROFILE=%s\n", PhoneMEInputGetProfileId(OverlayProfileSel));
    for (i = 0; i < OVERLAY_BIND_COUNT; i++) {
        fprintf(f, "%s=%d\n", OverlayBindNames[i], PhoneMEInputGetBind(i));
    }
    fclose(f);
}

static void overlay_init(void) {
    char *volumeText;

    OverlayConfigPath = getenv("PHONEME_CONFIG_PATH");
    overlay_apply_display();
    OverlayProfileSel = PhoneMEInputGetProfileIndex();
    volumeText = getenv("PHONEME_SOUND_VOLUME");
    if (volumeText != NULL && volumeText[0] != '\0') {
        OverlayVolume = atoi(volumeText);
    } else {
        OverlayVolume = MediaSDL_GetMasterVolume();
    }
    overlay_set_volume(OverlayVolume);
}

static int compute_fit_height_width(int srcWidth, int srcHeight) {
    return (srcWidth * SDL_FULLHEIGHT + (srcHeight / 2)) / srcHeight;
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

    haveActive = detect_active_bounds(surface, visibleWidth, visibleHeight,
                                      &activeX, &activeY, &activeW, &activeH);
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

static void compute_present_layout(int visibleWidth, int visibleHeight,
                                   PresentLayout *layout) {
    int fitWidth;

    layout->srcX = 0;
    layout->srcY = 0;
    layout->srcW = visibleWidth;
    layout->srcH = visibleHeight;
    layout->dstX = 0;
    layout->dstY = 0;
    layout->dstW = SDL_FULLWIDTH;
    layout->dstH = SDL_FULLHEIGHT;
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

    if (visibleWidth == SDL_FULLWIDTH && visibleHeight == 320) {
        layout->srcY = (visibleHeight - SDL_FULLHEIGHT) / 2;
        layout->srcH = SDL_FULLHEIGHT;
        layout->name = "crop-240x320";
        return;
    }

    if (visibleWidth == 176 && visibleHeight == 208) {
        layout->srcY = (visibleHeight - visibleWidth) / 2;
        layout->srcH = visibleWidth;
        layout->name = "crop-176x208";
        return;
    }

    if (visibleWidth == 176 && visibleHeight == 220) {
        fitWidth = compute_fit_height_width(visibleWidth, visibleHeight);
        layout->dstW = fitWidth;
        layout->dstX = (SDL_FULLWIDTH - fitWidth) / 2;
        layout->name = "fit-height-176x220";
        return;
    }

    if (visibleWidth == 128 && visibleHeight == 160) {
        fitWidth = compute_fit_height_width(visibleWidth, visibleHeight);
        layout->dstW = fitWidth;
        layout->dstX = (SDL_FULLWIDTH - fitWidth) / 2;
        layout->name = "fit-height-128x160";
        return;
    }

    if (visibleWidth == visibleHeight && visibleWidth <= 208) {
        layout->dstW = visibleWidth;
        layout->dstH = visibleHeight;
        layout->dstX = (SDL_FULLWIDTH - visibleWidth) / 2;
        layout->dstY = (SDL_FULLHEIGHT - visibleHeight) / 2;
        layout->name = "center-square";
        return;
    }

    if (visibleWidth == SDL_FULLWIDTH && visibleHeight == SDL_FULLHEIGHT) {
        layout->name = "native";
        return;
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
    const char *items[] = { "Resume", "Display", "Controls", "Sound", "Exit" };
    int i;
    for (i = 0; i < 5; i++) {
        overlay_draw_row(54 + i * 16, items[i], "", OverlayRow == i);
    }
}

static void overlay_draw_display(void) {
    overlay_draw_row(54, "Screen", OverlayModes[OverlayModeSel].label, OverlayRow == 0);
    overlay_draw_row(70, "Zoom", overlay_current_scale()->label, OverlayRow == 1);
    overlay_draw_row(86, "Size", OverlaySources[OverlaySourceSel].label, OverlayRow == 2);
}

static void overlay_draw_sound(void) {
    char value[32];
    snprintf(value, sizeof(value), "%d%%", OverlayVolume);
    overlay_draw_row(54, "Volume", value, OverlayRow == 0);
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

    boxColor(Native_SDL_Screen, 0, 0, SDL_FULLWIDTH - 1, SDL_FULLHEIGHT - 1,
             0x00000099);
    boxColor(Native_SDL_Screen, 8, 24, SDL_FULLWIDTH - 8, SDL_FULLHEIGHT - 12,
             0x101820ee);
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
    }

    stringColor(Native_SDL_Screen, 18, SDL_FULLHEIGHT - 20,
                "A:OK  B:Back  Power:Menu", 0x9aaab0ff);
}

static void overlay_adjust_selected(int direction) {
    if (OverlayPage == OVERLAY_PAGE_DISPLAY) {
        if (OverlayRow == 0) overlay_set_mode_index(OverlayModeSel + direction);
        else if (OverlayRow == 1) overlay_set_scale_index(OverlayScaleSel + direction);
        else if (OverlayRow == 2) overlay_set_source_index(OverlaySourceSel + direction);
        overlay_save_config();
    } else if (OverlayPage == OVERLAY_PAGE_SOUND) {
        overlay_set_volume(OverlayVolume + direction * 5);
        overlay_save_config();
    } else if (OverlayPage == OVERLAY_PAGE_CONTROLS) {
        if (OverlayRow == 0) {
            overlay_set_profile_index(OverlayProfileSel + direction);
            overlay_save_config();
        }
    }
}

static int overlay_row_count(void) {
    if (OverlayPage == OVERLAY_PAGE_MAIN) return 5;
    if (OverlayPage == OVERLAY_PAGE_DISPLAY) return 3;
    if (OverlayPage == OVERLAY_PAGE_SOUND) return 1;
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
    OverlayPage = OVERLAY_PAGE_MAIN;
    OverlayRow = 0;
}

static void overlay_accept(void) {
    if (OverlayPage == OVERLAY_PAGE_MAIN) {
        if (OverlayRow == 0) {
            OverlayVisible = 0;
            overlay_save_config();
        } else if (OverlayRow == 1) {
            OverlayPage = OVERLAY_PAGE_DISPLAY;
            OverlayRow = 0;
        } else if (OverlayRow == 2) {
            OverlayPage = OVERLAY_PAGE_CONTROLS;
            OverlayRow = 0;
        } else if (OverlayRow == 3) {
            OverlayPage = OVERLAY_PAGE_SOUND;
            OverlayRow = 0;
        } else if (OverlayRow == 4) {
            overlay_save_config();
            OverlayVisible = 0;
            OverlayShutdownRequested = 1;
        }
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

int PhoneMEOverlayConsumeShutdown(void) {
    if (!OverlayShutdownRequested) {
        return 0;
    }
    OverlayShutdownRequested = 0;
    return 1;
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
    int dstX, dstY;
    int srcX, srcY;
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
    SDL_FillRect(Native_SDL_Screen, NULL, 0);
    SDL_LockSurface(Native_SDL_Screen);

    srcPixels = (unsigned short *)visibleSurface->pixels;
    dstPixels = (unsigned short *)Native_SDL_Screen->pixels;
    srcPitchPixels = visibleSurface->pitch / 2;
    dstPitchPixels = Native_SDL_Screen->pitch / 2;

    for (dstY = 0; dstY < layout.dstH; dstY++) {
        srcY = layout.srcY + ((dstY * layout.srcH) / layout.dstH);
        for (dstX = 0; dstX < layout.dstW; dstX++) {
            srcX = layout.srcX + ((dstX * layout.srcW) / layout.dstW);
            dstPixels[(layout.dstY + dstY) * dstPitchPixels + (layout.dstX + dstX)] =
                srcPixels[srcY * srcPitchPixels + srcX];
        }
    }

    SDL_UnlockSurface(Native_SDL_Screen);
    overlay_draw();
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
    SDL_UpdateRect(Native_SDL_Screen, 0, 0, 0, 0);
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
{ static int refreshCount = 0;
  static int refreshVerbose = -1;
  static Uint32 lastRefreshLog = 0;
  Uint32 now;
  refreshCount++;
  if (refreshVerbose < 0) {
    refreshVerbose = (getenv("PHONEME_REFRESH_DEBUG") != NULL);
  }
  now = SDL_GetTicks();
  if ((refreshCount <= 8) ||
      (refreshVerbose && ((lastRefreshLog == 0) || ((now - lastRefreshLog) >= 1000)))) {
    fprintf(stderr, "lfjport_refresh: #%d area=%d,%d-%d,%d orient=%d original=%d ticks=%lu\n",
            refreshCount, x1, y1, x2, y2,
            Native_SDL_ScreenOrientation, OriginalOrientation,
            (unsigned long)now);
    lastRefreshLog = now;
  }
  update_refresh_bounds(x1, y1, x2, y2);
  SDL_UnlockSurface(Native_SDL_HScreen);
  SDL_UnlockSurface(Native_SDL_VScreen);
  if (Native_SDL_ScreenOrientation) {
      present_surface(Native_SDL_VScreen, OriginalOrientation == 0);
  } else {
      present_surface(Native_SDL_HScreen, OriginalOrientation == 1);
  }
  SDL_UpdateRect(Native_SDL_Screen, 0,0,0,0);
  SDL_Flip(Native_SDL_Screen);
  SDL_LockSurface(Native_SDL_HScreen);
  SDL_LockSurface(Native_SDL_VScreen);
  (void)x1;
  (void)y1;
  (void)x2;
  (void)y2;
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
