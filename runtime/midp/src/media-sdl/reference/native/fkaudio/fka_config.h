/*
 * Runtime settings for fkaudio: PhoneMEConfig() (core/phoneme_config) in
 * the runtime; plain getenv() when a file is compiled on its own, as the
 * reference tools in tools/doja/mldref do.
 */
#ifndef FKA_CONFIG_H
#define FKA_CONFIG_H

#include <stdlib.h>

#if defined(__has_include)
#if __has_include(<phoneme_config.h>)
#include <phoneme_config.h>
#define FKA_CONFIG(name) PhoneMEConfig(name)
#endif
#endif

#ifndef FKA_CONFIG
#define FKA_CONFIG(name) getenv(name)   /* standalone build of a single file */
#endif

#endif
