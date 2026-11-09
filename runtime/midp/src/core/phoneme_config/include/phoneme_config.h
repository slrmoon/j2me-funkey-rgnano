/*
 * phoneME FunKey runtime settings: the interface between a launcher
 * (launchers/<target>/) and the runtime.
 *
 * A launcher configures the runtime with environment variables. Every one
 * the runtime reads is listed, with its meaning, in phoneme_config.c, and
 * documented in docs/runtime-interface.md (tools/check-runtime-interface.py
 * keeps the three in step). Runtime code reads them only through
 * PhoneMEConfig(), never with getenv() directly.
 */
#ifndef PHONEME_CONFIG_H
#define PHONEME_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Value of a runtime setting, or NULL when the launcher did not set it
 * (exactly like getenv()). An unknown name still works but prints a warning
 * once, so a setting cannot be added without being listed.
 */
const char *PhoneMEConfig(const char *name);

/* the setting is set at all (any value, also "" or "0") */
int PhoneMEConfigIsSet(const char *name);

/* integer value, or def when unset or empty */
int PhoneMEConfigInt(const char *name, int def);

#ifdef __cplusplus
}
#endif

#endif
