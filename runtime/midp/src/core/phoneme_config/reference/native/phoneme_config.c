/*
 * phoneME FunKey runtime settings (see phoneme_config.h).
 *
 * The table is the list of everything a launcher can set. Keep it in step
 * with docs/runtime-interface.md: tools/check-runtime-interface.py checks
 * that both list the same names and that the runtime reads no others.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <phoneme_config.h>

typedef struct {
    const char *name;
    const char *meaning;
} PhoneMESetting;

static const PhoneMESetting Settings[] = {
    /* files and folders */
    { "MIDP_HOME",               "runtime folder with lib/ and appdb/" },
    { "PHONEME_RMS_HOME",        "folder for this game's record stores (saves)" },
    { "PHONEME_CONFIG_PATH",     "file the runtime menu saves display/sound/key choices to" },
    { "PHONEME_DEBUG_CONFIG",    "file with debug switches shared with the launcher" },
    { "PHONEME_RESTART_FLAG",    "file created when the user picks Restart in the runtime menu" },
    { "PHONEME_SOUNDBANK",       "sound bank file for the MIDI synthesizer" },
    /* memory */
    { "PHONEME_HEAP_MB",         "Java heap in MB" },
    { "PHONEME_HEAP_KB",         "Java heap in KB (wins over PHONEME_HEAP_MB)" },
    /* screen */
    { "PHONEME_LCD_SIZE",        "screen size the game sees, WxH" },
    { "PHONEME_SOURCE_SIZE",     "part of the game frame that is shown, WxH or full" },
    { "PHONEME_SCALE_MODE",      "auto, fit, fill, crop, center, zoom or integer" },
    { "PHONEME_SCALE_FILTER",    "sharp, smooth, pixel or scale2x" },
    { "PHONEME_SCALE_RATIO",     "zoom mode: 250..4000 (1/1000) or 25..400 (percent)" },
    { "PHONEME_PAN",             "zoom/crop: visible part, x,y in -1000..1000" },
    { "PHONEME_FPS_LIMIT",       "off, 20, 25, 30, 50 or 60" },
    { "PHONEME_SHOW_FPS",        "set: FPS counter on the screen" },
    { "J2ME_GP2X_REVERSE",       "set: rotate the screen by 90 degrees" },
    /* desktop window */
    { "PHONEME_DESKTOP",         "1: desktop window even at scale 1" },
    { "PHONEME_WINDOW_SCALE",    "desktop window: game screen times 1..8" },
    { "PHONEME_WINDOW_SIZE",     "desktop window size WxH (wins over the scale)" },
    { "PHONEME_FULLSCREEN",      "1: start in full screen (desktop)" },
    { "PHONEME_WINDOW_TITLE",    "desktop window title (UTF-8)" },
    /* keys */
    { "PHONEME_ENABLE_GP2X_KEYS","set: handheld button map (FunKey / RG Nano)" },
    { "PHONEME_KEY_PROFILE",     "handheld key profile id (universal, doja, nokia-s40, ...)" },
    { "PHONEME_IGNORE_JOYSTICK", "set: ignore joystick events" },
    { "PHONEME_PC_KEYS",         "PC keyboard map: SDL key = phone key code, ';'-separated" },
    { "J2ME_GP2X_JOYU",          "handheld button -> phone key code (up)" },
    { "J2ME_GP2X_JOYUL",         "handheld button -> phone key code (up-left)" },
    { "J2ME_GP2X_JOYL",          "handheld button -> phone key code (left)" },
    { "J2ME_GP2X_JOYDL",         "handheld button -> phone key code (down-left)" },
    { "J2ME_GP2X_JOYD",          "handheld button -> phone key code (down)" },
    { "J2ME_GP2X_JOYDR",         "handheld button -> phone key code (down-right)" },
    { "J2ME_GP2X_JOYR",          "handheld button -> phone key code (right)" },
    { "J2ME_GP2X_JOYUR",         "handheld button -> phone key code (up-right)" },
    { "J2ME_GP2X_JOYC",          "handheld button -> phone key code (stick click)" },
    { "J2ME_GP2X_START",         "handheld button -> phone key code (Start)" },
    { "J2ME_GP2X_SELECT",        "handheld button -> phone key code (Select)" },
    { "J2ME_GP2X_LEFT",          "handheld button -> phone key code (L shoulder)" },
    { "J2ME_GP2X_RIGHT",         "handheld button -> phone key code (R shoulder)" },
    { "J2ME_GP2X_BUTA",          "handheld button -> phone key code (A)" },
    { "J2ME_GP2X_BUTB",          "handheld button -> phone key code (B)" },
    { "J2ME_GP2X_BUTX",          "handheld button -> phone key code (X)" },
    { "J2ME_GP2X_BUTY",          "handheld button -> phone key code (Y)" },
    { "J2ME_GP2X_VOLU",          "handheld button -> phone key code (volume up)" },
    { "J2ME_GP2X_VOLD",          "handheld button -> phone key code (volume down)" },
    /* sound */
    { "PHONEME_ENABLE_AUDIO",    "set: sound on" },
    { "PHONEME_SOUND_VOLUME",    "master volume 0..100" },
    { "PHONEME_SYNTH",           "MIDI synthesizer: hybrid, wavetable or fm" },
    { "PHONEME_AUDIO_QUALITY",   "low or high: resampling quality, synth preset levels" },
    { "PHONEME_MIDI_GAIN",       "MIDI synthesizer gain 50..100" },
    { "PHONEME_AUDIO_BUFFER",    "audio buffer in samples, 128..8192" },
    { "PHONEME_AUDIO_SUM",       "1/0: mix stereo to mono (default: from /etc/asound.conf)" },
    { "PHONEME_MLD_EXACT",       "1: exact (slower) MLD (i-mode melody) synthesis" },
    { "PHONEME_MLD_MIDI",        "1: play MLD melodies through the MIDI synthesizer" },
    /* logs and debugging */
    { "PHONEME_DISPLAY_DEBUG",   "set: log display and scaling decisions" },
    { "PHONEME_KEY_DEBUG",       "set: log keys" },
    { "PHONEME_EVENT_DEBUG",     "set: log events sent to the game" },
    { "PHONEME_AUDIO_TRACE",     "set: log the audio device setup" },
    { "PHONEME_PROFILE",         "1: sampling CPU profiler (Linux)" },
    { "PHONEME_PROFILE_SECONDS", "profiler report period in seconds" }
};

#define SETTING_COUNT ((int) (sizeof(Settings) / sizeof(Settings[0])))

static int known(const char *name) {
    int i;
    for (i = 0; i < SETTING_COUNT; i++) {
        if (strcmp(Settings[i].name, name) == 0) return 1;
    }
    return 0;
}

const char *PhoneMEConfig(const char *name) {
    static const char *warned[8];
    static int warnedCount;
    if (name == NULL) return NULL;
    if (!known(name)) {
        int i, seen = 0;
        for (i = 0; i < warnedCount; i++) if (warned[i] == name) seen = 1;
        if (!seen) {
            if (warnedCount < 8) warned[warnedCount++] = name;
            fprintf(stderr, "phoneME: setting %s is not listed in phoneme_config.c\n", name);
        }
    }
    return getenv(name);
}

int PhoneMEConfigIsSet(const char *name) {
    return PhoneMEConfig(name) != NULL;
}

int PhoneMEConfigInt(const char *name, int def) {
    const char *v = PhoneMEConfig(name);
    return v != NULL && v[0] != '\0' ? atoi(v) : def;
}
