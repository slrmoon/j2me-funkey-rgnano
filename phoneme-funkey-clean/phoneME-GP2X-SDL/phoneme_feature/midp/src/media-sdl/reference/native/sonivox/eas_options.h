/*
 * Sonivox EAS build configuration for the FunKey S / RG Nano runtime.
 *
 * 22050 Hz mono output with 8-bit wavetable samples: the configuration EAS
 * shipped with on handsets (the Hybrid FM + wavetable engine requires it).
 * All three engines are built in and selected at run time.
 */
#ifndef EAS_OPTIONS_CMAKE
#define EAS_OPTIONS_CMAKE

#define UNIFIED_DEBUG_MESSAGES
#define EAS_WT_SYNTH
#define EAS_FM_SYNTH
#define EAS_HYBRID_SYNTH
#define NUM_OUTPUT_CHANNELS 1
#define MAX_SYNTH_VOICES 64
/* one EAS stream + virtual synthesizer per playing MIDI Player */
#define MAX_NUMBER_STREAMS 16
#define MAX_VIRTUAL_SYNTHESIZERS 16
#define _FILTER_ENABLED
#define DLS_SYNTHESIZER
/* compiled in (the SF2 loader needs them) but switched off at run time:
 * phones of the era had neither reverb nor chorus on the ringer path */
#define _REVERB_ENABLED
#define _CHORUS_ENABLED
#define _XMF_PARSER
#define _RMID_PARSER
#define _8_BIT_SAMPLES
#define _SAMPLE_RATE_22050
#define _ZLIB_UNPACKER
#define _SF2_SUPPORT
#define _FLOAT_DCF

#endif
