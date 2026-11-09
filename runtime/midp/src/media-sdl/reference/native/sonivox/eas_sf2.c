// SoundFont 2.04 File Parser
//
// Note that this is not a complete implementation. It only supports what EAS's DLS synth can do.
// Unsupported features are reported in the log.
//
// SF3 (https://github.com/FluidSynth/fluidsynth/wiki/SoundFont3Format) is recognized (with the sample link flag correctly set),
// though not supported, and will not load.

#include "eas_options.h"
#include "eas_sf2.h"
#include "eas_host.h"
#include "eas_report.h"
#include "eas_mdls.h"

#include <stdlib.h>
#include <string.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

enum {
    RIFF_IDENTIFIER = 0x52494646, // 'RIFF'
    LIST_IDENTIFIER = 0x4C495354, // 'LIST'
    SFBK_IDENTIFIER = 0x7366626B, // 'sfbk'
    INFO_IDENTIFIER = 0x494E464F, // 'INFO'
    SDTA_IDENTIFIER = 0x73647461, // 'sdta'
    SMPL_IDENTIFIER = 0x736D706C, // 'smpl'
    SM24_IDENTIFIER = 0x736D3234, // 'sm24'
    PDTA_IDENTIFIER = 0x70647461, // 'pdta'
    PHDR_IDENTIFIER = 0x70686472, // 'phdr'
    PBAG_IDENTIFIER = 0x70626167, // 'pbag'
    PMOD_IDENTIFIER = 0x706D6F64, // 'pmod'
    PGEN_IDENTIFIER = 0x7067656E, // 'pgen'
    INST_IDENTIFIER = 0x696E7374, // 'inst'
    IBAG_IDENTIFIER = 0x69626167, // 'ibag'
    IMOD_IDENTIFIER = 0x696D6F64, // 'imod'
    IGEN_IDENTIFIER = 0x6967656E, // 'igen'
    SHDR_IDENTIFIER = 0x73686472, // 'shdr'
};

// SFGenerator enums
enum E_SFGenerator {
    sfg_startAddrsOffset = 0,
	sfg_endAddrsOffset = 1,
	sfg_startloopAddrsOffset = 2,
	sfg_endloopAddrsOffset = 3,
	sfg_startAddrsCoarseOffset = 4,
	sfg_modLfoToPitch = 5,
	sfg_vibLfoToPitch = 6,
	sfg_modEnvToPitch = 7,
	sfg_initialFilterFc = 8,
	sfg_initialFilterQ = 9,
	sfg_modLfoToFilterFc = 10,
	sfg_modEnvToFilterFc = 11,
    sfg_endAddrsCoarseOffset = 12,
	sfg_modLfoToVolume = 13,
	sfg_unused1 = 14,
	sfg_chorusEffectsSend = 15,
	sfg_reverbEffectsSend = 16,
	sfg_pan = 17,
	sfg_unused2 = 18,
	sfg_unused3 = 19,
	sfg_unused4 = 20,
	sfg_delayModLFO = 21,
	sfg_freqModLFO = 22,
	sfg_delayVibLFO = 23,
	sfg_freqVibLFO = 24,
	sfg_delayModEnv = 25,
	sfg_attackModEnv = 26,
	sfg_holdModEnv = 27,
	sfg_decayModEnv = 28,
	sfg_sustainModEnv = 29,
	sfg_releaseModEnv = 30,
	sfg_keynumToModEnvHold = 31,
	sfg_keynumToModEnvDecay = 32,
	sfg_delayVolEnv = 33,
	sfg_attackVolEnv = 34,
	sfg_holdVolEnv = 35,
	sfg_decayVolEnv = 36,
	sfg_sustainVolEnv = 37,
	sfg_releaseVolEnv = 38,
	sfg_keynumToVolEnvHold = 39,
	sfg_keynumToVolEnvDecay = 40,
	sfg_instrument = 41,
	sfg_reserved1 = 42,
	sfg_keyRange = 43,
	sfg_velRange = 44,
	sfg_startloopAddrsCoarseOffset = 45,
	sfg_keynum = 46,
    sfg_velocity = 47,
	sfg_initialAttenuation = 48,
	sfg_reserved2 = 49,
	sfg_endloopAddrsCoarseOffset = 50,
	sfg_coarseTune = 51,
	sfg_fineTune = 52,
	sfg_sampleID = 53,
	sfg_sampleModes = 54,
	sfg_reserved3 = 55,
	sfg_scaleTuning = 56,
	sfg_exclusiveClass = 57,
	sfg_overridingRootKey = 58,
	sfg_unused5 = 59,
	sfg_endOper = 60
};

// SFModulator enums
enum E_SFModulator {
	sfm_noController = 0,
	sfm_noteOnVolume = 2,
	sfm_noteOnKeyNumber = 3,
	sfm_polyPressure = 10,
	sfm_channelPressure = 13,
	sfm_pitchWheel = 14,
	sfm_pitchWheelSensitivity = 16,
	sfm_link = 127
};

// SFTransform enums
enum E_SFTransform { 
	sft_Linear = 0,
	sft_AbsoluteValue = 2
};

// SFControllerType enums
enum E_SFControllerType { 
	sfct_Linear = 0, 
	sfct_Concave = 1, 
	sfct_Convex = 2, 
	sfct_Switch = 3 
};

enum E_SF2_REC_SIZES {
    PHDR_SIZE = 38,
    SHDR_SIZE = 46,
    INST_SIZE = 22,
    IBAG_SIZE = 4,
    PBAG_SIZE = 4,
    MOD_SIZE = 10,
    GEN_SIZE = 4,
};

enum E_SF2Parser_LIMITS {
    MAX_GENS_IN_MERGED_REGION = 65536,
    MAX_MODS_IN_MERGED_REGION = 65536
};

enum E_SAMPLE_TYPE_FLAGS {
    SAMPLETYPE_COMPRESSED = 0x10
};

// internal data when parsing
typedef struct S_SF2_PARSER
{
    EAS_HW_DATA_HANDLE hwInstData;
    EAS_FILE_HANDLE fileHandle;
    EAS_I32 fileOffset;
    EAS_U32 fileSize;

    S_DLS* pDLS;

    EAS_U32 smplOffset; // points to the start of raw data
    EAS_U32 smplSize; // in bytes

    EAS_U32 sampleCount;
    EAS_U32 sampleDLSSize; // size in S_DLS
    EAS_U32 sampleDLSWritten; // in bytes
    EAS_U32* pSampleDLSOffsets; // in bytes
    EAS_U32* pSampleDLSLens; // in bytes
    EAS_SAMPLE* pSampleDLS;
    struct S_SF2_SAMPLE {
        EAS_I16 staticTuning;
        EAS_U8 rootKey;
        // all in samples
        EAS_U32 start;
        EAS_U32 end; // past the end
        EAS_U32 loopStart;
        EAS_U32 loopEnd; // past the end
        EAS_U16 sampleType;
    }* pShdrs;

    EAS_I32 phdrOffset;
    EAS_I32 pbagOffset;
    EAS_U32 pbagCount; // exclude terminal entry
    EAS_I32 pmodOffset;
    EAS_I32 pgenOffset;

    EAS_I32 instOffset;
    EAS_I32 ibagOffset;
    EAS_U32 ibagCount; // exclude terminal entry
    EAS_I32 imodOffset;
    EAS_I32 igenOffset;

    EAS_I32 shdrOffest;

    struct S_SF2_INST {
        EAS_U16 regionIndex;
        EAS_U16 regionCount;
        EAS_U32* pSharedArticulationIndices; // multiple preset can have one instrument without pmod or pgen, so its articulations could be shared
                                             // size of this array is regionCount
    } * pInsts;
    EAS_U32 instCount;

    struct S_SF2_REGION {
        EAS_U32 genOffset; // relative to igen or pgen offset
        EAS_U16 genCount;
        EAS_U32 modOffset; // relative to imod or pmod offset
        EAS_U16 modCount;
        EAS_BOOL isGlobal;
        EAS_U8 keyLow;
        EAS_U8 keyHigh;
        EAS_U8 velLow;
        EAS_U8 velHigh;
        EAS_U16 waveIndex; // it could point to an instrument or sample
    } * pInstRegions;
    EAS_U32 instRegionCount;
    EAS_U32 instRegionWritten; // used in parsing ibags

    S_PROGRAM* pPresets;
    S_DLS_REGION* pPresetRegions;
    S_DLS_ARTICULATION* pPresetArticulations;
    EAS_U32 presetCount;
    EAS_U32 presetRegionCount;
    EAS_U32 presetRegionWritten;
    EAS_U32 presetArticulationCount;
    EAS_U32 presetArticulationWritten;
} S_SF2_PARSER;

const EAS_U16 defaultGenerators[sfg_endOper] = {
    0,    // startAddrsOffset
    0,    // endAddrsOffset
    0,    // startloopAddrsOffset
    0,    // endloopAddrsOffset
    0,    // startAddrsCoarseOffset
    0,    // modLfoToPitch
    0,    // vibLfoToPitch
    0,    // modEnvToPitch
    13500,    // initialFilterFc
    0,    // initialFilterQ
    0,    // modLfoToFilterFc
    0,    // modEnvToFilterFc
    0,    // endAddrsCoarseOffset
    0,    // modLfoToVolume
    0,    // unused1
    0,    // chorusEffectsSend
    0,    // reverbEffectsSend
    0,    // pan
    0,    // unused2
    0,    // unused3
    0,    // unused4
    -12000,    // delayModLFO
    0,    // freqModLFO
    -12000,    // delayVibLFO
    0,    // freqVibLFO
    -12000,    // delayModEnv
    -12000,    // attackModEnv
    -12000,    // holdModEnv
    -12000,    // decayModEnv
    0,    // sustainModEnv
    -12000,    // releaseModEnv
    0,    // keynumToModEnvHold
    0,    // keynumToModEnvDecay
    -12000,    // delayVolEnv
    -12000,    // attackVolEnv
    -12000,    // holdVolEnv
    -12000,    // decayVolEnv
    0,    // sustainVolEnv
    -12000,    // releaseVolEnv
    0,    // keynumToVolEnvHold
    0,    // keynumToVolEnvDecay
    -1,   // instrument
    0,    // reserved1
    0x7F00,    // keyRange
    0x7F00,    // velRange
    0,    // startloopAddrsCoarseOffset
    -1,    // keynum
    -1,    // velocity
    0,    // initialAttenuation
    0,   // reserved2
    0,    // endloopAddrsCoarseOffset
    0,    // coarseTune
    0,    // fineTune
    -1,   // sampleID
    0,    // sampleModes
    0,    // reserved3
    100,    // scaleTuning
    0,    // exclusiveClass
    -1,    // overridingRootKey
    0    // unused5
};

// pNextChunkPos must point to the first chunk when reading it
static EAS_RESULT RIFFNextChunk(S_SF2_PARSER* pParser, EAS_U32* pChunkType, EAS_U32* pChunkSize, EAS_I32* pNextChunkPos)
{
    EAS_RESULT result;

    result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, *pNextChunkPos);
    if (result != EAS_SUCCESS) {
        return result;
    }
    
    result = EAS_HWGetDWord(pParser->hwInstData, pParser->fileHandle, pChunkType, EAS_TRUE);
    if (result != EAS_SUCCESS) {
        return result;
    }

    result = EAS_HWGetDWord(pParser->hwInstData, pParser->fileHandle, pChunkSize, EAS_FALSE);
    if (result != EAS_SUCCESS) {
        return result;
    }

    *pNextChunkPos += 8;
    *pNextChunkPos += *pChunkSize;

    // Adjust to word boundary
    if (*pNextChunkPos % 2 != 0) {
        (*pNextChunkPos)++;
    }

    return EAS_SUCCESS;
}

static EAS_RESULT Parse_sdta(S_SF2_PARSER* pParser, EAS_I32 offset, EAS_U32 size);
static EAS_RESULT Parse_pdta(S_SF2_PARSER* pParser, EAS_I32 offset, EAS_U32 size);

static EAS_RESULT Parse_shdrs(S_SF2_PARSER* pParser, EAS_BOOL dryRun);

static EAS_RESULT Parse_samples(S_SF2_PARSER* pParser, EAS_BOOL dryRun);

// offset is file offset to the first bag record
static EAS_RESULT Parse_ibags(S_SF2_PARSER* pParser, EAS_I32 offset, EAS_U32 bagCount);
static EAS_RESULT Parse_pbags(S_SF2_PARSER* pParser, EAS_I32 offset, EAS_U32 bagCount, EAS_BOOL dryRun);

static EAS_RESULT Parse_region(S_SF2_PARSER* pParser, struct S_SF2_REGION* pRegion, EAS_I32 genChunkOffset);

static EAS_RESULT Parse_bag(S_SF2_PARSER* pParser, const EAS_I16 gens[sfg_endOper], void* pMods, EAS_U32 modCount, S_DLS_ARTICULATION* pDLSArt, S_DLS_REGION* pDLSRegion, EAS_U8* loopMode);

static EAS_RESULT Parse_Insts(S_SF2_PARSER* pParser, EAS_BOOL dryRun);
static EAS_RESULT Parse_Presets(S_SF2_PARSER* pParser, EAS_BOOL dryRun);

static inline EAS_RESULT Check_gen(EAS_U16 genType, EAS_BOOL isInst)
{
    if (isInst) { // igen
        if (genType == sfg_instrument) {
            EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Found preset-only generator type %u at igen entry\n", genType);
            return EAS_ERROR_FILE_FORMAT;
        }
    } else { // pgen
        switch (genType) {
        case sfg_startAddrsOffset:
        case sfg_endAddrsOffset:
        case sfg_startloopAddrsOffset:
        case sfg_endloopAddrsOffset:
        case sfg_startAddrsCoarseOffset:
        case sfg_endAddrsCoarseOffset:
        case sfg_startloopAddrsCoarseOffset:
        case sfg_keynum:
        case sfg_velocity:
        case sfg_endloopAddrsCoarseOffset:
        case sfg_sampleID:
        case sfg_sampleModes:
        case sfg_exclusiveClass:
        case sfg_overridingRootKey:
            EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Found inst-only generator type %u at pgen entry\n", genType);
            return EAS_ERROR_FILE_FORMAT;
        default:
            break;
        }
    }
    return EAS_SUCCESS;
}

EAS_RESULT SF2Parser (EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE fileHandle, EAS_I32 offset, S_DLS **ppDLS)
{
    S_SF2_PARSER parser;
    EAS_RESULT result;
    memset(&parser, 0, sizeof(S_SF2_PARSER));

    *ppDLS = NULL;

    parser.hwInstData = hwInstData;
    parser.fileHandle = fileHandle;
    parser.fileOffset = offset;

    EAS_I32 nextChunkPos = offset;
    EAS_U32 temp;

    result = RIFFNextChunk(&parser, &temp, &parser.fileSize, &nextChunkPos);
    if (result != EAS_SUCCESS) {
        return result;
    }
    parser.fileSize += 8; // include RIFF header size
    if (temp != RIFF_IDENTIFIER) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Not a RIFF file (%08x)\n", temp);
        return EAS_ERROR_FILE_FORMAT;
    }

    result = EAS_HWGetDWord(parser.hwInstData, parser.fileHandle, &temp, EAS_TRUE);
    if (result != EAS_SUCCESS) {
        return result;
    }
    if (temp != SFBK_IDENTIFIER) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Not a SoundFont file (%08x)\n", temp);
        return EAS_ERROR_FILE_FORMAT;
    }

    // Read chunks
    nextChunkPos = offset + 12; // RIFF <size> sfbk
    while (1) {
        if (parser.fileOffset + parser.fileSize - nextChunkPos <= 1)  {
            break; // end of file
        }

        EAS_U32 chunkType, chunkSize;
        result = RIFFNextChunk(&parser, &chunkType, &chunkSize, &nextChunkPos);
        if (result != EAS_SUCCESS) {
            return result;
        }

        EAS_I32 curpos;
        result = EAS_HWFilePos(parser.hwInstData, parser.fileHandle, &curpos);
        if (result != EAS_SUCCESS) {
            return result;
        }

        if ((EAS_U32)curpos >= parser.fileOffset + parser.fileSize) {
            break; // end of file
        }

        switch (chunkType) {
        case LIST_IDENTIFIER:
            result = EAS_HWGetDWord(parser.hwInstData, parser.fileHandle, &temp, EAS_TRUE);
            if (result != EAS_SUCCESS) {
                return result;
            }
            switch (temp) {
            case INFO_IDENTIFIER:
                break;
            case SDTA_IDENTIFIER:
                result = Parse_sdta(&parser, curpos, chunkSize);
                if (result != EAS_SUCCESS) {
                    EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse sdta LIST chunk: %ld\n", result);
                    return result;
                }
                break;
            case PDTA_IDENTIFIER:
                result = Parse_pdta(&parser, curpos, chunkSize);
                if (result != EAS_SUCCESS) {
                    EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse pdta LIST chunk: %ld\n", result);
                    return result;
                }
                break;
            default:
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Unknown LIST chunk type %08x\n", temp);
                break;
            }
            break;
        default:
            EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Unknown chunk type %08x\n", chunkType);
            break;
        }
    }

    // stage 1: instrument level, dry run
    if (parser.shdrOffest == 0) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: No shdr chunk found in pdta LIST\n");
        return EAS_ERROR_FILE_FORMAT;
    }
    // get sample count and sample data size in S_DLS
    result = Parse_shdrs(&parser, EAS_TRUE);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse shdrs (dry): %ld\n", result);
        return result;
    }

    if (parser.instOffset == 0) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: No inst chunk found in pdta LIST\n");
        return EAS_ERROR_FILE_FORMAT;
    }
    // get instrument and regions count
    result = Parse_Insts(&parser, EAS_TRUE);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse insts (dry): %ld\n", result);
        return result;
    }

    // stage 2: instrument level, full run
    // 1. parse shdrs
    parser.pShdrs = EAS_HWMalloc(parser.hwInstData, sizeof(struct S_SF2_SAMPLE) * parser.sampleCount);
    if (parser.pShdrs == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate memory for shdr structs (%lu bytes)\n",
                   (unsigned long)(sizeof(struct S_SF2_SAMPLE) * parser.sampleCount));
        return EAS_ERROR_MALLOC_FAILED;
    }
    result = Parse_shdrs(&parser, EAS_FALSE);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse shdrs: %ld\n", result);
        return result;
    }
    // 2. parse insts
    parser.pInsts = EAS_HWMalloc(parser.hwInstData, sizeof(struct S_SF2_INST) * parser.instCount);
    if (parser.pInsts == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate memory for inst structs (%lu bytes)\n",
                   (unsigned long)(sizeof(struct S_SF2_INST) * parser.instCount));
        return EAS_ERROR_MALLOC_FAILED;
    }
    parser.pInstRegions = EAS_HWMalloc(parser.hwInstData, sizeof(struct S_SF2_REGION) * parser.instRegionCount);
    if (parser.pInstRegions == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate memory for inst region structs (%lu bytes)\n",
                   (unsigned long)(sizeof(S_DLS_REGION) * parser.instRegionCount));
        return EAS_ERROR_MALLOC_FAILED;
    }
    result = Parse_Insts(&parser, EAS_FALSE);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse insts: %ld\n", result);
        return result;
    }

    // stage 3: preset level, dry run
    if (parser.phdrOffset == 0) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: No phdr found in pdta LIST\n");
        return EAS_ERROR_FILE_FORMAT;
    }

    result = Parse_Presets(&parser, EAS_TRUE);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse presets (dry): %ld\n", result);
        return result;
    }

    // stage 4: preset level, full run
    parser.pPresets = EAS_HWMalloc(parser.hwInstData, sizeof(S_PROGRAM) * parser.presetCount);
    if (parser.pPresets == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate memory for preset program structs (%lu bytes)\n",
                   (unsigned long)(sizeof(S_PROGRAM) * parser.presetCount));
        return EAS_ERROR_MALLOC_FAILED;
    }
    parser.pPresetRegions = EAS_HWMalloc(parser.hwInstData, sizeof(S_DLS_REGION) * parser.presetRegionCount);
    if (parser.pPresetRegions == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate memory for preset region structs (%lu bytes)\n",
                   (unsigned long)(sizeof(S_DLS_REGION) * parser.presetRegionCount));
        return EAS_ERROR_MALLOC_FAILED;
    }
    parser.pPresetArticulations = EAS_HWMalloc(parser.hwInstData, sizeof(S_DLS_ARTICULATION) * parser.presetArticulationCount);
    if (parser.pPresetArticulations == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate memory for preset articulation structs (%lu bytes)\n",
                   (unsigned long)(sizeof(S_DLS_ARTICULATION) * parser.presetArticulationCount));
        return EAS_ERROR_MALLOC_FAILED;
    }
    result = Parse_Presets(&parser, EAS_FALSE);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse presets: %ld\n", result);
        return result;
    }

    // stage 5: samples
    parser.pSampleDLSOffsets = EAS_HWMalloc(parser.hwInstData, sizeof(EAS_U32) * parser.sampleCount);
    if (parser.pSampleDLSOffsets == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate memory for sample offsets (%lu bytes)\n",
                   (unsigned long)(sizeof(EAS_U32) * parser.sampleCount));
        return EAS_ERROR_MALLOC_FAILED;
    }

    parser.pSampleDLSLens = EAS_HWMalloc(parser.hwInstData, sizeof(EAS_U32) * parser.sampleCount);
    if (parser.pSampleDLSLens == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate memory for sample lengths (%lu bytes)\n",
                   (unsigned long)(sizeof(EAS_U32) * parser.sampleCount));
        return EAS_ERROR_MALLOC_FAILED;
    }

    result = Parse_samples(&parser, EAS_TRUE);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse samples (dry): %ld\n", result);
        return result;
    }

    parser.pSampleDLS = EAS_HWMalloc(parser.hwInstData, parser.sampleDLSSize);
    if (parser.pSampleDLS == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate memory for sample data (%lu bytes)\n",
                   (unsigned long)parser.sampleDLSSize);
        return EAS_ERROR_MALLOC_FAILED;
    }

    result = Parse_samples(&parser, EAS_FALSE);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse samples: %ld\n", result);
        return result;
    }

    // stage 6: copy everything to S_DLS
    parser.pDLS = EAS_HWMalloc(hwInstData, sizeof(S_DLS));
    if (parser.pDLS == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate memory for DLS structure\n");
        return EAS_ERROR_MALLOC_FAILED;
    }
    memset(parser.pDLS, 0, sizeof(S_DLS));
    parser.pDLS->pDLSPrograms = parser.pPresets;
    parser.pDLS->pDLSRegions = parser.pPresetRegions;
    parser.pDLS->pDLSArticulations = parser.pPresetArticulations;
    parser.pDLS->pDLSSampleLen = parser.pSampleDLSLens;
    parser.pDLS->pDLSSampleOffsets = parser.pSampleDLSOffsets;
    parser.pDLS->pDLSSamples = parser.pSampleDLS;
    parser.pDLS->numDLSPrograms = parser.presetCount;
    parser.pDLS->numDLSRegions = parser.presetRegionCount;
    parser.pDLS->numDLSArticulations = parser.presetArticulationCount;
    parser.pDLS->numDLSSamples = parser.sampleCount;
    parser.pDLS->refCount = 1;
    parser.pDLS->libType = DLSLIB_TYPE_SF2;

    *ppDLS = parser.pDLS;

    // stage 7: cleanup
    EAS_HWFree(hwInstData, parser.pShdrs);
    EAS_HWFree(hwInstData, parser.pInsts);
    EAS_HWFree(hwInstData, parser.pInstRegions);

    return EAS_SUCCESS;
}

EAS_RESULT SF2Cleanup (EAS_HW_DATA_HANDLE hwInstData, S_DLS *pDLS)
{
    if (!pDLS) {
        return EAS_SUCCESS;
    }

    if (pDLS->libType != DLSLIB_TYPE_SF2) {
        return EAS_ERROR_DATA_INCONSISTENCY;
    }

    EAS_HWFree(hwInstData, pDLS->pDLSPrograms);
    EAS_HWFree(hwInstData, pDLS->pDLSRegions);
    EAS_HWFree(hwInstData, pDLS->pDLSArticulations);
    EAS_HWFree(hwInstData, pDLS->pDLSSampleLen);
    EAS_HWFree(hwInstData, pDLS->pDLSSampleOffsets);
    EAS_HWFree(hwInstData, pDLS->pDLSSamples);
    EAS_HWFree(hwInstData, pDLS);
    return EAS_SUCCESS;
}

static EAS_RESULT Parse_sdta(S_SF2_PARSER* pParser, EAS_I32 offset, EAS_U32 size)
{
    EAS_RESULT result;
    EAS_I32 pos = offset + 4; // skip 'sdta' identifier
    
    while (1) {
        if (offset + size - pos <= 1) {
            break; // end of LIST chunk
        }

        EAS_U32 chunkType, chunkSize;
        result = RIFFNextChunk(pParser, &chunkType, &chunkSize, &pos);
        if (result != EAS_SUCCESS) {
            return result;
        }

        EAS_I32 curpos;
        result = EAS_HWFilePos(pParser->hwInstData, pParser->fileHandle, &curpos);
        if (result != EAS_SUCCESS) {
            return result;
        }
        if ((EAS_U32)curpos >= offset + size) {
            break; // end of LIST chunk
        }

        switch (chunkType) {
        case SMPL_IDENTIFIER:
            if (chunkSize % 2 == 1) {
                EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: smpl chunk size is not even (%u)\n", chunkSize);
                return EAS_ERROR_FILE_FORMAT;
            }
            pParser->smplOffset = curpos;
            pParser->smplSize = chunkSize;
            break;
        case SM24_IDENTIFIER:
            // EAS don't support 24 bit samples, throw it away
            break;
        default:
            EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Unknown chunk in sdta LIST: %08x\n", chunkType);
            break;
        }
    }

    if (pParser->smplSize == 0) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: No smpl chunk found in sdta LIST\n");
        return EAS_ERROR_FILE_FORMAT;
    }

    return EAS_SUCCESS;
}

static EAS_RESULT Decode_SF2smpl(S_SF2_PARSER* pParser, struct S_SF2_SAMPLE* pShdr, EAS_U32 writeOffset, EAS_U32* wroteLength, EAS_BOOL dryRun)
{
    EAS_RESULT result;

    EAS_U32 nSamples = pShdr->end - pShdr->start; // in samples

    if (pShdr->loopStart >= pShdr->end || pShdr->loopStart < pShdr->start) {
        EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Invalid loop start %u out of range [%u, %u) of sample %u\n", pShdr->loopStart, pShdr->start, pShdr->end, (unsigned)(pShdr - pParser->pShdrs));
        pShdr->loopStart = pShdr->start;
    }
    if (pShdr->loopEnd > pShdr->end || pShdr->loopEnd <= pShdr->start) {
        EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Invalid loop end %u out of range [%u, %u) of sample %u\n", pShdr->loopEnd, pShdr->start, pShdr->end, (unsigned)(pShdr - pParser->pShdrs));
        pShdr->loopEnd = pShdr->end;
    }

    if (pShdr->loopStart != pShdr->loopEnd) {
        // reserved for copying *loopStart to 1 beyond loopEnd, see WT_Interpolate
        nSamples += 1;
    }

    const EAS_U32 dlsLen = nSamples * sizeof(EAS_SAMPLE);
    const EAS_U32 dataLen = nSamples * 2;

    if (dryRun) {
        *wroteLength = dlsLen;
        return EAS_SUCCESS;
    }
    
    result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, pParser->smplOffset + (pShdr->start * 2));
    if (result != EAS_SUCCESS) {
        return result;
    }

    EAS_U8 buffer[4096];
    EAS_U32 remaining = dataLen;
    EAS_SAMPLE* destination = pParser->pSampleDLS + (writeOffset / sizeof(EAS_SAMPLE));
    while (remaining > 0) {
        EAS_U32 c = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
        result = EAS_HWReadFile(pParser->hwInstData, pParser->fileHandle, buffer, c, &c);
        if (result != EAS_SUCCESS) {
            return result;
        }
#ifdef _16_BIT_SAMPLES
        memcpy(destination, buffer, c);
#else
        for (EAS_U32 j = 0; j < c / 2; j++) {
            ((EAS_SAMPLE*)destination)[j] = buffer[j * 2 + 1]; // MSB
        }
#endif
        remaining -= c;
        destination += c / 2;
    }

    if (pShdr->loopStart != pShdr->loopEnd) {
        // copy *loopStart to 1 beyond loopEnd, see WT_Interpolate
        pParser->pSampleDLS[(writeOffset / sizeof(EAS_SAMPLE)) + (pShdr->loopEnd - pShdr->start)] = pParser->pSampleDLS[(writeOffset / sizeof(EAS_SAMPLE)) + (pShdr->loopStart - pShdr->start)];
    }

    *wroteLength = dlsLen;
    return result;
}

static EAS_RESULT Parse_samples(S_SF2_PARSER* pParser, EAS_BOOL dryRun)
{
    EAS_RESULT result;

    if (dryRun) {
        pParser->sampleDLSSize = 0;
    }
    pParser->sampleDLSWritten = 0;

    for (EAS_U32 i = 0; i < pParser->sampleCount; i++) {
        struct S_SF2_SAMPLE* pShdr = &pParser->pShdrs[i];

        EAS_U32 dlsLen = 0;

        pParser->pSampleDLSOffsets[i] = pParser->sampleDLSWritten;

        if (pShdr->sampleType & SAMPLETYPE_COMPRESSED) {
            // only vorbis is supported now
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Compressed sample data is not supported (sample %u)\n", (unsigned)i);
            return EAS_ERROR_FEATURE_NOT_AVAILABLE;
        }
        result = Decode_SF2smpl(pParser, pShdr, pParser->pSampleDLSOffsets[i], &dlsLen, dryRun);
        if (result != EAS_SUCCESS) {
            return result;
        }
        pParser->pSampleDLSLens[i] = dlsLen;
        if (dryRun) {
            pParser->sampleDLSSize += dlsLen;
        } else {
            pParser->sampleDLSWritten += dlsLen;
        }
    }

    return EAS_SUCCESS;
}

static EAS_RESULT Parse_pdta(S_SF2_PARSER* pParser, EAS_I32 offset, EAS_U32 size)
{
    EAS_RESULT result;
    EAS_I32 pos = offset + 4; // skip 'pdta' identifier

    while (1) {
        if (offset + size - pos <= 1) {
            break; // end of LIST chunk
        }

        EAS_U32 chunkType, chunkSize;
        result = RIFFNextChunk(pParser, &chunkType, &chunkSize, &pos);
        if (result != EAS_SUCCESS) {
            return result;
        }

        EAS_I32 curpos;
        result = EAS_HWFilePos(pParser->hwInstData, pParser->fileHandle, &curpos);
        if (result != EAS_SUCCESS) {
            return result;
        }
        if ((EAS_U32)curpos >= offset + size) {
            break; // end of LIST chunk
        }

        switch (chunkType) {
        case PHDR_IDENTIFIER:
            pParser->phdrOffset = curpos - 8; // at the start of the chunk ('phdr' identifier)
            break;
        case PBAG_IDENTIFIER:
            pParser->pbagOffset = curpos - 8;
            pParser->pbagCount = (chunkSize / PBAG_SIZE) - 1;
            break;
        case PMOD_IDENTIFIER:
            pParser->pmodOffset = curpos - 8;
            break;
        case PGEN_IDENTIFIER:
            pParser->pgenOffset = curpos - 8;
            break;
        case INST_IDENTIFIER:
            pParser->instOffset = curpos - 8;
            break;
        case IBAG_IDENTIFIER:
            pParser->ibagOffset = curpos - 8;
            pParser->ibagCount = (chunkSize / IBAG_SIZE) - 1;
            break;
        case IMOD_IDENTIFIER:
            pParser->imodOffset = curpos - 8;
            break;
        case IGEN_IDENTIFIER:
            pParser->igenOffset = curpos - 8;
            break;
        case SHDR_IDENTIFIER:
            pParser->shdrOffest = curpos - 8;
            break;
        default:
            EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Unknown chunk in pdta at 0x%lx: 0x%08x\n", (long)curpos, chunkType);
            break;
        }
    }
    return EAS_SUCCESS;
}

static EAS_RESULT Parse_shdrs(S_SF2_PARSER* pParser, EAS_BOOL dryRun) {
    EAS_RESULT result;
    EAS_I32 pos = pParser->shdrOffest;

    EAS_U32 tmp, chunkSize;
    result = RIFFNextChunk(pParser, &tmp, &chunkSize, &pos);
    if (result != EAS_SUCCESS) {
        return result;
    }
    if (tmp != SHDR_IDENTIFIER) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Expected 'shdr' chunk, got 0x%08x\n", tmp);
        return EAS_ERROR_FILE_FORMAT;
    }
    if (chunkSize % SHDR_SIZE != 0) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Invalid SHDR chunk size %u\n", chunkSize);
        return EAS_ERROR_FILE_FORMAT;
    }

    pParser->sampleCount = chunkSize / SHDR_SIZE - 1; // exclude EOS
    if (dryRun) {
        return EAS_SUCCESS;
    }

    for (EAS_U32 i = 0; i < pParser->sampleCount; i++) {
        // skip sample name
        result = EAS_HWFileSeekOfs(pParser->hwInstData, pParser->fileHandle, 20);
        if (result != EAS_SUCCESS) {
            return result;
        }
        result = EAS_HWGetDWord(pParser->hwInstData, pParser->fileHandle, &tmp, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        pParser->pShdrs[i].start = tmp;
        result = EAS_HWGetDWord(pParser->hwInstData, pParser->fileHandle, &tmp, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        pParser->pShdrs[i].end = tmp;
        result = EAS_HWGetDWord(pParser->hwInstData, pParser->fileHandle, &tmp, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        pParser->pShdrs[i].loopStart = tmp;
        result = EAS_HWGetDWord(pParser->hwInstData, pParser->fileHandle, &tmp, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        pParser->pShdrs[i].loopEnd = tmp;
        result = EAS_HWGetDWord(pParser->hwInstData, pParser->fileHandle, &tmp, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        pParser->pShdrs[i].staticTuning = DLSConvertSampleRate(tmp);

        EAS_U8 tmp2;
        result = EAS_HWGetByte(pParser->hwInstData, pParser->fileHandle, &tmp2);
        if (result != EAS_SUCCESS) {
            return result;
        }
        pParser->pShdrs[i].rootKey = tmp2;
        result = EAS_HWGetByte(pParser->hwInstData, pParser->fileHandle, &tmp2);
        if (result != EAS_SUCCESS) {
            return result;
        }
        pParser->pShdrs[i].staticTuning += (EAS_I8)tmp2;
        // skip sample link
        result = EAS_HWFileSeekOfs(pParser->hwInstData, pParser->fileHandle, 2);
        if (result != EAS_SUCCESS) {
            return result;
        }
        EAS_U16 sampleType;
        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &sampleType, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        pParser->pShdrs[i].sampleType = sampleType;
    }

    return EAS_SUCCESS;
}

static EAS_RESULT Parse_Insts(S_SF2_PARSER* pParser, EAS_BOOL dryRun)
{
    EAS_RESULT result;
    EAS_I32 pos = pParser->instOffset;

    EAS_U32 tmp, chunkSize;
    result = RIFFNextChunk(pParser, &tmp, &chunkSize, &pos);
    if (result != EAS_SUCCESS) {
        return result;
    }
    if (tmp != INST_IDENTIFIER) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Expected 'inst' chunk, got %08x\n", tmp);
        return EAS_ERROR_FILE_FORMAT;
    }
    if (chunkSize % INST_SIZE != 0) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Invalid inst chunk size %u\n", chunkSize);
        return EAS_ERROR_FILE_FORMAT;
    }

    pParser->instCount = chunkSize / INST_SIZE - 1; // exclude the 'EOI' entry
    if (dryRun) {
        pParser->instRegionCount = 0;
    }
    pParser->instRegionWritten = 0;

    EAS_U16 bagIndex;
    EAS_U16 prevBagIndex = 0;
    for (EAS_U32 i = 0; i < pParser->instCount + 1; i++) {
        const long offset = pParser->instOffset + 8 + (INST_SIZE * i);
        // skip inst name
        result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, offset + 20);
        if (result != EAS_SUCCESS) {
            return result;
        }
        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &bagIndex, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        if (i == 0) {
            goto skip;
        }
        if (i == pParser->instCount) {
            // EOI's bagIndex is 0
            bagIndex = pParser->ibagCount;
        }

        if (bagIndex == prevBagIndex) {
            // empty inst
            goto skip;
        }
        if (bagIndex < prevBagIndex) {
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Non monotonic bag index %u at inst entry at 0x%lx\n", bagIndex, offset);
            return EAS_ERROR_FILE_FORMAT;
        }
        
        if (dryRun) {
            pParser->instRegionCount += bagIndex - prevBagIndex;
            goto skip;
        }

        pParser->pInsts[i - 1].regionIndex = pParser->instRegionWritten;
        pParser->pInsts[i - 1].regionCount = bagIndex - prevBagIndex;

        result = Parse_ibags(pParser, pParser->ibagOffset + 8 + (prevBagIndex * IBAG_SIZE), bagIndex - prevBagIndex);
        if (result != EAS_SUCCESS) {
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse bags of inst entry at 0x%lx: %ld\n", offset - INST_SIZE, result);
            return result;
        }
    skip:
        prevBagIndex = bagIndex;
    }

    return EAS_SUCCESS;
}

static EAS_RESULT Parse_region(S_SF2_PARSER* pParser, struct S_SF2_REGION* pRegion, EAS_I32 genChunkOffset)
{
    EAS_RESULT result;
    pRegion->velLow = 0;
    pRegion->velHigh = 127;
    pRegion->keyLow = 0;
    pRegion->keyHigh = 127;
    pRegion->isGlobal = EAS_TRUE;

    for (EAS_U32 j = 0; j < pRegion->genCount; j++) {
        result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, genChunkOffset + pRegion->genOffset + (j * GEN_SIZE));
        if (result != EAS_SUCCESS) {
            return result;
        }
        EAS_U16 genType, genValue;
        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genType, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genValue, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        // genValue is read as LE
        if (genType == sfg_keyRange) {
            // some weird sf2 specify keyrange on global regions
            // pRegion->isGlobal = EAS_FALSE;
            pRegion->keyLow = (EAS_U8)(genValue & 0xFF);
            pRegion->keyHigh = (EAS_U8)((genValue >> 8) & 0xFF);
        } else if (genType == sfg_velRange) {
            // pRegion->isGlobal = EAS_FALSE;
            pRegion->velLow = (EAS_U8)(genValue & 0xFF);
            pRegion->velHigh = (EAS_U8)((genValue >> 8) & 0xFF);
        } else if (genType == sfg_instrument) {
            pRegion->isGlobal = EAS_FALSE;
            pRegion->waveIndex = genValue;
        } else if (genType == sfg_sampleID) {
            pRegion->isGlobal = EAS_FALSE;
            pRegion->waveIndex = genValue;
        }
    }

    return EAS_SUCCESS;
}

static EAS_RESULT Parse_ibags(S_SF2_PARSER* pParser, EAS_I32 offset, EAS_U32 bagCount)
{
    EAS_RESULT result;

    if (bagCount == 0) {
        return EAS_SUCCESS; // no bags to parse
    }

    EAS_U16 genIndex, prevGenIndex = 0;
    EAS_U16 modIndex, prevModIndex = 0;
    for (EAS_U32 i = 0; i < bagCount + 1; i++) {
        const long myoffset = offset + (i * IBAG_SIZE);
        result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, myoffset);
        if (result != EAS_SUCCESS) {
            return result;
        }
        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genIndex, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modIndex, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        if (i == 0) {
            goto skip;
        }

        if (genIndex < prevGenIndex) {
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Non monotonic generator index %u at ibag entry at 0x%lx\n", genIndex, myoffset);
            return EAS_ERROR_FILE_FORMAT;
        }
        if (modIndex < prevModIndex) {
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Non monotonic modulator index %u at ibag entry at 0x%lx\n", modIndex, myoffset);
            return EAS_ERROR_FILE_FORMAT;
        }

        EAS_U32 regionIndex = pParser->instRegionWritten;

        pParser->pInstRegions[regionIndex].genOffset = 8 + (prevGenIndex * GEN_SIZE);
        pParser->pInstRegions[regionIndex].modOffset = 8 + (prevModIndex * MOD_SIZE);
        pParser->pInstRegions[regionIndex].genCount = genIndex - prevGenIndex;
        pParser->pInstRegions[regionIndex].modCount = modIndex - prevModIndex;

        result = Parse_region(pParser, &pParser->pInstRegions[regionIndex], pParser->igenOffset);
        if (result != EAS_SUCCESS) {
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to ibag entry at 0x%lx: %ld\n", myoffset - IBAG_SIZE, result);
            return result;
        }
        pParser->instRegionWritten++;
    skip:
        prevGenIndex = genIndex;
        prevModIndex = modIndex;
    }

    return EAS_SUCCESS;
}

static EAS_RESULT Parse_Presets(S_SF2_PARSER* pParser, EAS_BOOL dryRun)
{
    EAS_RESULT result;
    EAS_I32 pos = pParser->phdrOffset;

    EAS_U32 tmp, chunkSize;
    result = RIFFNextChunk(pParser, &tmp, &chunkSize, &pos);
    if (result != EAS_SUCCESS) {
        return result;
    }
    if (tmp != PHDR_IDENTIFIER) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Expected 'phdr' chunk, got %08x\n", tmp);
        return EAS_ERROR_FILE_FORMAT;
    }
    if (chunkSize % PHDR_SIZE != 0) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Invalid phdr chunk size %u\n", chunkSize);
        return EAS_ERROR_FILE_FORMAT;
    }

    pParser->presetCount = chunkSize / PHDR_SIZE - 1; // exclude the 'EOP' entry
    if (dryRun) {
        pParser->presetRegionCount = 0;
        pParser->presetArticulationCount = 0;
    }
    pParser->presetRegionWritten = 0;
    pParser->presetArticulationWritten = 0;

    EAS_U16 bagIndex, prevBagIndex = 0;
    for (EAS_U32 i = 0; i < pParser->presetCount + 1; i++) {
        const long offset = pParser->phdrOffset + 8 + (PHDR_SIZE * i);
        // skip inst name
        result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, offset + 20);
        if (result != EAS_SUCCESS) {
            return result;
        }

        EAS_U16 presetID, bankID;
        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &presetID, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &bankID, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }

        if (bankID > 128) {
            EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Invalid bankID %u at phdr entry at 0x%lx, ignoring\n", bankID, offset);
            bankID = 0xFF;
        }

        if (bankID == 128) {
            bankID = DEFAULT_RHYTHM_BANK_MSB | 0x100; // rhythm flag
        }

        if (presetID > 128) {
            EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Invalid presetID %u at phdr entry at 0x%lx, ignoring\n", presetID, offset);
            presetID = 0xFF;
        }

        if (!dryRun && i != pParser->presetCount) {
            pParser->pPresets[i].locale = (bankID << 16) | presetID;
        }

        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &bagIndex, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        if (i == 0) {
            goto skip;
        }
        if (i == pParser->presetCount) {
            // EOP's bagIndex is 0
            bagIndex = pParser->pbagCount;
        }
        if (bagIndex == prevBagIndex) {
            // empty inst
            goto skip;
        }

        if (bagIndex < prevBagIndex) {
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Non monotonic bag index %u at phdr entry at 0x%lx\n", bagIndex, offset);
            return EAS_ERROR_FILE_FORMAT;
        }

        EAS_U32 regionIndex = pParser->presetRegionWritten;
        result = Parse_pbags(pParser, pParser->pbagOffset + 8 + (prevBagIndex * 4), bagIndex - prevBagIndex, dryRun);
        if (result != EAS_SUCCESS) {
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse pbags at entry at 0x%lx: %ld\n", offset - PHDR_SIZE, result);
            return result;
        }

        if (!dryRun) {
            if (pParser->presetRegionWritten == regionIndex) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Preset %u has no regions\n", i - 1);
                pParser->pPresets[i - 1].regionIndex = INVALID_REGION_INDEX;
                goto skip;
            }
            EAS_U32 regionLast = pParser->presetRegionWritten - 1;
            pParser->pPresets[i - 1].regionIndex = regionIndex | FLAG_RGN_IDX_DLS_SYNTH;
            pParser->pPresetRegions[regionLast].wtRegion.region.keyGroupAndFlags |= REGION_FLAG_LAST_REGION;
        }
    skip:
        prevBagIndex = bagIndex;
    }

    if (dryRun) {
        return EAS_SUCCESS;
    }

    if (pParser->presetArticulationWritten != pParser->presetArticulationCount) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: written articulation count %u does not match calculated count %u\n", pParser->presetArticulationWritten, pParser->presetArticulationCount);
        return EAS_ERROR_DATA_INCONSISTENCY;
    }

    if (pParser->presetRegionWritten != pParser->presetRegionCount) {
        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: written region count %u does not match calculated count %u\n", pParser->presetRegionWritten, pParser->presetRegionCount);
        return EAS_ERROR_DATA_INCONSISTENCY;
    }

    return EAS_SUCCESS;
}

inline static EAS_BOOL IsValueGenerator(EAS_U16 gen) {
    return (gen >= 5 && gen != 12 && gen <= 13)
        || (gen >= 15 && gen <= 17)
        || (gen >= 21 && gen <= 40)
        || (gen == 48)
        || (gen == 51)
        || (gen == 52);
}

static EAS_RESULT Parse_pbags(S_SF2_PARSER* pParser, EAS_I32 offset, EAS_U32 bagCount, EAS_BOOL dryRun)
{
    EAS_RESULT result;

    if (bagCount == 0) {
        return EAS_SUCCESS; // no bags to parse
    }

    // isGlobal is used to check if global region exists
    struct S_SF2_REGION presetGlobalRegion = {0, 0, 0, 0, EAS_FALSE, 0, 0, 0, 0};

    EAS_U16 genIndex, prevGenIndex = 0;
    EAS_U16 modIndex, prevModIndex = 0;
    for (EAS_U32 i = 0; i < bagCount + 1; i++) {
        const long myOffset = offset + (i * PBAG_SIZE);
        result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, myOffset);
        if (result != EAS_SUCCESS) {
            return result;
        }
        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genIndex, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modIndex, EAS_FALSE);
        if (result != EAS_SUCCESS) {
            return result;
        }
        if (i == 0) {
            goto skip;
        }

        if (genIndex < prevGenIndex) {
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Non monotonic generator index %u at ibag entry at %lu\n", genIndex, myOffset);
            return EAS_ERROR_FILE_FORMAT;
        }
        if (modIndex < prevModIndex) {
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Non monotonic modulator index %u at ibag entry at %lu\n", modIndex, myOffset);
            return EAS_ERROR_FILE_FORMAT;
        }

        // this is parsing region (i-1) now

        struct S_SF2_REGION presetRegion;
        presetRegion.genOffset = 8 + (prevGenIndex * GEN_SIZE);
        presetRegion.modOffset = 8 + (prevModIndex * MOD_SIZE);
        presetRegion.genCount = genIndex - prevGenIndex;
        presetRegion.modCount = modIndex - prevModIndex;

        result = Parse_region(pParser, &presetRegion, pParser->pgenOffset);
        if (result != EAS_SUCCESS) {
            EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse pbag at entry %u: %ld\n", i - 1, result);
            return result;
        }

        if (presetRegion.isGlobal) {
            presetGlobalRegion = presetRegion;
            goto skip;
        }

        // merge preset's global region and current region, inst's global region and matching regions
        struct S_SF2_INST* pInst = &pParser->pInsts[presetRegion.waveIndex];
        struct S_SF2_REGION* pInstGlobalRegion = NULL;
        for (EAS_U32 instRegionIndex = 0; instRegionIndex < pInst->regionCount; instRegionIndex++) {
            struct S_SF2_REGION* pInstRegion = &pParser->pInstRegions[pInst->regionIndex + instRegionIndex];
            if (pInstRegion->isGlobal) {
                pInstGlobalRegion = pInstRegion;
                continue;
            }

            EAS_U8 keyL = max(presetRegion.keyLow, pInstRegion->keyLow);
            EAS_U8 keyH = min(presetRegion.keyHigh, pInstRegion->keyHigh);
            if (keyL > keyH) {
                continue; // no overlap
            }
            EAS_U8 velL = max(presetRegion.velLow, pInstRegion->velLow);
            EAS_U8 velH = min(presetRegion.velHigh, pInstRegion->velHigh);
            if (velL > velH) {
                continue; // no overlap
            }

            // whether a new articulation should be created for this region
            EAS_BOOL createArticulation = presetRegion.modCount > 0 || presetRegion.genCount > 0 || presetGlobalRegion.isGlobal;
            // whether to share the newly created articulation
            EAS_BOOL shareArticulation = EAS_FALSE;
            if (!createArticulation) {
                if (pInst->pSharedArticulationIndices == NULL) {
                    pInst->pSharedArticulationIndices = EAS_HWMalloc(pParser->hwInstData, sizeof(EAS_U32) * pInst->regionCount);
                    if (pInst->pSharedArticulationIndices == NULL) {
                        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to allocate shared articulation indices for inst %u\n", presetRegion.waveIndex);
                        return EAS_ERROR_MALLOC_FAILED;
                    }
                    memset(pInst->pSharedArticulationIndices, 0xFF, sizeof(EAS_U32) * pInst->regionCount);
                }
                if (pInst->pSharedArticulationIndices[instRegionIndex] == 0xFFFFFFFF) {
                    createArticulation = EAS_TRUE;
                    shareArticulation = EAS_TRUE;
                }
            }

            if (dryRun) {
                pParser->presetRegionCount++;
                if (createArticulation) {
                    pParser->presetArticulationCount++;
                }
                continue;
            }

            struct s_mod {
                EAS_U16 src;
                EAS_U16 dest;
                EAS_U16 amount;
                EAS_U16 amountSrc;
                EAS_U16 transform;
            } mods[MAX_MODS_IN_MERGED_REGION];
            EAS_U32 modCount = 6;
            EAS_I16 gens[sfg_endOper];
            memcpy(gens, defaultGenerators, sizeof(defaultGenerators));

            // default modulators
            // note-on vel to attenuation
            mods[0].src = 0x0502;
            mods[0].dest = sfg_initialAttenuation;
            mods[0].amount = 960;
            mods[0].amountSrc = 0;
            mods[0].transform = 0;

            // note-on vel to filter cutoff
            mods[1].src = 0x0102;
            mods[1].dest = sfg_initialFilterFc;
            mods[1].amount = -2400;
            mods[1].amountSrc = 0;
            mods[1].transform = 0;

            // channel pressure to vib lfo pitch depth
            mods[2].src = 0x000D;
            mods[2].dest = sfg_vibLfoToPitch;
            mods[2].amount = 50;
            mods[2].amountSrc = 0;
            mods[2].transform = 0;

            // cc1 to vib lfo pitch depth
            mods[3].src = 0x0081;
            mods[3].dest = sfg_vibLfoToPitch;
            mods[3].amount = 50;
            mods[3].amountSrc = 0;
            mods[3].transform = 0;

            // cc91 to reverb send
            mods[4].src = 0x00DB;
            mods[4].dest = sfg_reverbEffectsSend;
            mods[4].amount = 200;
            mods[4].amountSrc = 0;
            mods[4].transform = 0;

            // cc93 to chorus send
            mods[5].src = 0x00DD;
            mods[5].dest = sfg_chorusEffectsSend;
            mods[5].amount = 200;
            mods[5].amountSrc = 0;
            mods[5].transform = 0;

            // N.B. this is ordered, latter will overwrite former
            // apply inst global
            if (pInstGlobalRegion != NULL) {
                for (EAS_U32 k = 0; k < pInstGlobalRegion->genCount; k++) {
                    EAS_U16 genType;
                    EAS_I16 genValue;
                    result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, pParser->igenOffset + pInstGlobalRegion->genOffset + (k * GEN_SIZE));
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genType, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genValue, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = Check_gen(genType, EAS_TRUE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    gens[genType] = genValue;
                }

                for (EAS_U32 k = 0; k < pInstGlobalRegion->modCount; k++) {
                    if (modCount >= MAX_MODS_IN_MERGED_REGION) {
                        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: too many modulators in merged preset %u region %u\n", i - 1, instRegionIndex);
                        return EAS_ERROR_SOUND_LIBRARY;
                    }
                    EAS_U16 modSrc;
                    EAS_U16 modDest;
                    EAS_U16 modAmount;
                    EAS_U16 modAmountSrc;
                    EAS_U16 modTransform;
                    result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, pParser->imodOffset + pInstGlobalRegion->modOffset + (k * MOD_SIZE));
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modSrc, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modDest, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modAmount, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modAmountSrc, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modTransform, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }

                    for (EAS_U32 l = 0; l < modCount; l++) {
                        if (mods[l].src == modSrc && mods[l].dest == modDest && mods[l].amountSrc == modAmountSrc && mods[l].transform == modTransform) {
                            mods[l].amount = modAmount;
                            goto foundoverride;
                        }
                    }
                    mods[modCount].src = modSrc;
                    mods[modCount].dest = modDest;
                    mods[modCount].amount = modAmount;
                    mods[modCount].amountSrc = modAmountSrc;
                    mods[modCount].transform = modTransform;
                    modCount++;
                foundoverride:;
                }
            }

            // apply inst region
            // this should add sampleID
            EAS_BOOL foundSampleID = EAS_FALSE;
            EAS_U16 sampleID;
            for (EAS_U32 k = 0; k < pInstRegion->genCount; k++) {
                EAS_U16 genType;
                EAS_I16 genValue;
                result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, pParser->igenOffset + pInstRegion->genOffset + (k * GEN_SIZE));
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genType, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genValue, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                if (genType == sfg_keyRange || genType == sfg_velRange) {
                    continue;
                }
                if (genType == sfg_sampleID) {
                    foundSampleID = EAS_TRUE;
                    sampleID = genValue;
                    continue;
                }
                result = Check_gen(genType, EAS_TRUE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                gens[genType] = genValue;
            }

            if (!foundSampleID) {
                EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Inst %u region %u does not contain sampleID\n", presetRegion.waveIndex, instRegionIndex);
                return EAS_FAILURE;
            }

            for (EAS_U32 k = 0; k < pInstRegion->modCount; k++) {
                if (modCount >= MAX_MODS_IN_MERGED_REGION) {
                    EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: too many modulators in merged preset %u region %u\n", i - 1, instRegionIndex);
                    return EAS_ERROR_SOUND_LIBRARY;
                }
                EAS_U16 modSrc;
                EAS_U16 modDest;
                EAS_U16 modAmount;
                EAS_U16 modAmountSrc;
                EAS_U16 modTransform;
                result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, pParser->imodOffset + pInstRegion->modOffset + (k * MOD_SIZE));
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modSrc, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modDest, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modAmount, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modAmountSrc, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modTransform, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }

                for (EAS_U32 l = 0; l < modCount; l++) {
                    if (mods[l].src == modSrc && mods[l].dest == modDest && mods[l].amountSrc == modAmountSrc && mods[l].transform == modTransform) {
                        mods[l].amount = modAmount;
                        goto foundoverride2;
                    }
                }
                mods[modCount].src = modSrc;
                mods[modCount].dest = modDest;
                mods[modCount].amount = modAmount;
                mods[modCount].amountSrc = modAmountSrc;
                mods[modCount].transform = modTransform;
                modCount++;
            foundoverride2:;
            }

            // apply preset global
            if (presetGlobalRegion.isGlobal) {
                for (EAS_U32 k = 0; k < presetGlobalRegion.genCount; k++) {
                    EAS_U16 genType;
                    EAS_I16 genValue;
                    result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, pParser->pgenOffset + presetGlobalRegion.genOffset + (k * GEN_SIZE));
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genType, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genValue, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = Check_gen(genType, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    if (IsValueGenerator(genType)) {
                        gens[genType] += genValue;
                    } else {
                        gens[genType] = genValue;
                    }
                }

                for (EAS_U32 k = 0; k < presetGlobalRegion.modCount; k++) {
                    if (modCount >= MAX_MODS_IN_MERGED_REGION) {
                        EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: too many modulators in merged preset %u region %u\n", i - 1, instRegionIndex);
                        return EAS_ERROR_SOUND_LIBRARY;
                    }
                    EAS_U16 modSrc;
                    EAS_U16 modDest;
                    EAS_U16 modAmount;
                    EAS_U16 modAmountSrc;
                    EAS_U16 modTransform;
                    result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, pParser->pmodOffset + presetGlobalRegion.modOffset + (k * MOD_SIZE));
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modSrc, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modDest, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modAmount, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modAmountSrc, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }
                    result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modTransform, EAS_FALSE);
                    if (result != EAS_SUCCESS) {
                        return result;
                    }

                    mods[modCount].src = modSrc;
                    mods[modCount].dest = modDest;
                    mods[modCount].amount = modAmount;
                    mods[modCount].amountSrc = modAmountSrc;
                    mods[modCount].transform = modTransform;
                    modCount++;
                }
            }

            // apply preset region
            for (EAS_U32 k = 0; k < presetRegion.genCount; k++) {
                EAS_U16 genType;
                EAS_I16 genValue;
                result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, pParser->pgenOffset + presetRegion.genOffset + (k * GEN_SIZE));
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genType, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &genValue, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                if (genType == sfg_keyRange || genType == sfg_velRange || genType == sfg_instrument) {
                    continue;
                }
                result = Check_gen(genType, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                if (IsValueGenerator(genType)) {
                    gens[genType] += genValue;
                } else {
                    gens[genType] = genValue;
                }
            }

            for (EAS_U32 k = 0; k < presetRegion.modCount; k++) {
                if (modCount >= MAX_MODS_IN_MERGED_REGION) {
                    EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: too many modulators in merged preset %u region %u\n", i - 1, instRegionIndex);
                    return EAS_ERROR_SOUND_LIBRARY;
                }
                EAS_U16 modSrc;
                EAS_U16 modDest;
                EAS_U16 modAmount;
                EAS_U16 modAmountSrc;
                EAS_U16 modTransform;
                result = EAS_HWFileSeek(pParser->hwInstData, pParser->fileHandle, pParser->pmodOffset + presetRegion.modOffset + (k * MOD_SIZE));
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modSrc, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modDest, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modAmount, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modAmountSrc, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }
                result = EAS_HWGetWord(pParser->hwInstData, pParser->fileHandle, &modTransform, EAS_FALSE);
                if (result != EAS_SUCCESS) {
                    return result;
                }

                mods[modCount].src = modSrc;
                mods[modCount].dest = modDest;
                mods[modCount].amount = modAmount;
                mods[modCount].amountSrc = modAmountSrc;
                mods[modCount].transform = modTransform;
                modCount++;
            }

            // apply range
            gens[sfg_keyRange] = keyL | (keyH << 8); // LE
            gens[sfg_velRange] = velL | (velH << 8); // LE

            // parse
            EAS_U32 regionIndex = pParser->presetRegionWritten;
            EAS_U32 artIndex = pParser->presetArticulationWritten;

            if (!createArticulation) {
                artIndex = pInst->pSharedArticulationIndices[instRegionIndex];
            }

            if (artIndex >= pParser->presetArticulationCount) {
                EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: written articulation count exceeds calculated count %u\n", (unsigned)pParser->presetArticulationCount);
                return EAS_ERROR_DATA_INCONSISTENCY;
            }

            if (pParser->presetRegionWritten >= pParser->presetRegionCount) {
                EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: written region count exceeds calculated count %u\n", (unsigned)pParser->presetRegionCount);
                return EAS_ERROR_DATA_INCONSISTENCY;
            }

            EAS_I16 rootKey = gens[sfg_overridingRootKey];
            EAS_U8 loopMode;
            result = Parse_bag(pParser, gens, mods, modCount, &pParser->pPresetArticulations[artIndex], &pParser->pPresetRegions[regionIndex], &loopMode);
            if (result != EAS_SUCCESS) {
                EAS_Report(_EAS_SEVERITY_ERROR, "SF2Parser: Failed to parse merged region at pbag entry at 0x%lx\n", myOffset - PBAG_SIZE);
                return result;
            }

            pParser->pPresetRegions[regionIndex].wtRegion.artIndex = artIndex;
            pParser->pPresetRegions[regionIndex].wtRegion.waveIndex = sampleID;
            if (rootKey == -1) {
                rootKey = pParser->pShdrs[sampleID].rootKey;
            }
            if (loopMode != 0) {
                pParser->pPresetRegions[regionIndex].wtRegion.loopStart += pParser->pShdrs[sampleID].loopStart - pParser->pShdrs[sampleID].start;
                pParser->pPresetRegions[regionIndex].wtRegion.loopEnd += pParser->pShdrs[sampleID].loopEnd - pParser->pShdrs[sampleID].start;
            } else {
                pParser->pPresetRegions[regionIndex].wtRegion.loopStart = 0;
                pParser->pPresetRegions[regionIndex].wtRegion.loopEnd = 0;
            }
            if (pParser->pPresetRegions[regionIndex].wtRegion.loopStart != pParser->pPresetRegions[regionIndex].wtRegion.loopEnd) {
                pParser->pPresetRegions[regionIndex].wtRegion.region.keyGroupAndFlags |= REGION_FLAG_IS_LOOPED;
            }
            pParser->pPresetRegions[regionIndex].wtRegion.tuning = pParser->pShdrs[sampleID].staticTuning;
            pParser->pPresetRegions[regionIndex].wtRegion.tuning -= rootKey * pParser->pPresetArticulations[artIndex].keyNumToPitch / 128; // convert to cents, 1 semitone = 100 cents

            if (shareArticulation) {
                pInst->pSharedArticulationIndices[instRegionIndex] = artIndex;
            }

            if (createArticulation) {
                pParser->presetArticulationWritten++;
            }
            pParser->presetRegionWritten++;
        } // for each region of presetregion's inst

    skip:
        prevGenIndex = genIndex;
        prevModIndex = modIndex;
    } // for each region of preset

    return EAS_SUCCESS;
}

struct s_mod_src {
    EAS_U8 index;
    EAS_BOOL cc;
    EAS_BOOL direction;
    EAS_BOOL polarity;
    EAS_U8 type;
};

inline static struct s_mod_src ModSrc(EAS_U16 src) {
    struct s_mod_src tag;
    tag.index = src & 0x3F;
    tag.cc = (src & 0x80) != 0;
    tag.direction = (src & 0x100) != 0;
    tag.polarity = (src & 0x200) != 0;
    tag.type = (src >> 10) & 0x7F;
    return tag;
}

static EAS_RESULT Parse_bag(S_SF2_PARSER* pParser, const EAS_I16 gens[sfg_endOper], void* pMods, EAS_U32 modCount, S_DLS_ARTICULATION* pDLSArt, S_DLS_REGION* pDLSRegion, EAS_U8* loopMode)
{
    *loopMode = 0; // no loop

    if (gens[sfg_startAddrsOffset] != 0 || gens[sfg_endAddrsOffset] != 0 || gens[sfg_startAddrsCoarseOffset] != 0 || gens[sfg_endAddrsCoarseOffset] != 0) {
        EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: sample addrs offset is not supported\n");
    }

    pDLSArt->modLFO.lfoDelay = -DLSConvertDelay(gens[sfg_delayModLFO]);
    pDLSArt->modLFO.lfoFreq = DLSConvertPitchToPhaseInc(gens[sfg_freqModLFO]);

    pDLSArt->vibLFO.lfoDelay = -DLSConvertDelay(gens[sfg_delayVibLFO]);
    pDLSArt->vibLFO.lfoFreq = DLSConvertPitchToPhaseInc(gens[sfg_freqVibLFO]);

    pDLSArt->eg1.delayTime = DLSConvertDelay(gens[sfg_delayVolEnv]);
    pDLSArt->eg1.attackTime = gens[sfg_attackVolEnv];
    pDLSArt->eg1.holdTime = gens[sfg_holdVolEnv];
    pDLSArt->eg1.decayTime = gens[sfg_decayVolEnv];
    pDLSArt->eg1.sustainLevel = DLSConvertSustain(1000 - gens[sfg_sustainVolEnv]); // EG1 itself is in decibels
    pDLSArt->eg1.releaseTime = DLSConvertDelay(gens[sfg_releaseVolEnv]);
    pDLSArt->eg1.velToAttack = 0;
    pDLSArt->eg1.keyNumToHold = -gens[sfg_keynumToVolEnvHold] * 128;
    pDLSArt->eg1.keyNumToDecay = -gens[sfg_keynumToVolEnvDecay] * 128;
    pDLSArt->eg1ShutdownTime = 0;

    pDLSArt->eg2.delayTime = DLSConvertDelay(gens[sfg_delayModEnv]);
    pDLSArt->eg2.attackTime = gens[sfg_attackModEnv];
    pDLSArt->eg2.holdTime = gens[sfg_holdModEnv];
    pDLSArt->eg2.decayTime = gens[sfg_decayModEnv];
    pDLSArt->eg2.sustainLevel = DLSConvertSustain(1000 - gens[sfg_sustainModEnv]);
    pDLSArt->eg2.releaseTime = DLSConvertDelay(gens[sfg_releaseModEnv]);
    pDLSArt->eg2.velToAttack = 0;
    pDLSArt->eg2.keyNumToHold = -gens[sfg_keynumToModEnvHold] * 128;
    pDLSArt->eg2.keyNumToDecay = -gens[sfg_keynumToModEnvDecay] * 128;

    // Key 60 have unmodified decay and hold time
    pDLSArt->eg1.decayTime -= pDLSArt->eg1.keyNumToDecay / 128 * 60;
    pDLSArt->eg1.holdTime -= pDLSArt->eg1.keyNumToHold / 128 * 60;
    pDLSArt->eg2.decayTime -= pDLSArt->eg2.keyNumToDecay / 128 * 60;
    pDLSArt->eg2.holdTime -= pDLSArt->eg2.keyNumToHold / 128 * 60;

    pDLSArt->filterCutoff = gens[sfg_initialFilterFc];
    pDLSArt->filterQandFlags = DLSConvertQ(gens[sfg_initialFilterQ]);
    pDLSArt->modLFOToFc = gens[sfg_modLfoToFilterFc];
    pDLSArt->modLFOCC1ToFc = 0;
    pDLSArt->modLFOChanPressToFc = 0;
    pDLSArt->eg2ToFc = gens[sfg_modEnvToFilterFc];
    pDLSArt->velToFc = 0;
    pDLSArt->keyNumToFc = 0;

    pDLSArt->modLFOToGain = gens[sfg_modLfoToVolume];
    pDLSArt->modLFOCC1ToGain = 0;
    pDLSArt->modLFOChanPressToGain = 0;

    pDLSArt->tuning = gens[sfg_coarseTune] * 100 + gens[sfg_fineTune]; // 100 cents = 1 semitone
    pDLSArt->keyNumToPitch = 12800 * gens[sfg_scaleTuning] / 100;
    pDLSArt->vibLFOToPitch = gens[sfg_vibLfoToPitch];
    pDLSArt->vibLFOCC1ToPitch = 0;
    pDLSArt->vibLFOChanPressToPitch = 0;
    pDLSArt->modLFOToPitch = gens[sfg_modLfoToPitch];
    pDLSArt->eg2ToPitch = gens[sfg_modEnvToPitch];

    pDLSArt->pan = DLSConvertPan(gens[sfg_pan]);
    
#ifdef _CC_CHORUS
    pDLSArt->chorusSend = gens[sfg_chorusEffectsSend];
    pDLSArt->cc91ToReverbSend = 0;
#endif
#ifdef _CC_REVERB
    pDLSArt->reverbSend = gens[sfg_reverbEffectsSend];
    pDLSArt->cc93ToChorusSend = 0;
#endif

    // here we don't know loop region of the sample, so just put offsets here
    pDLSRegion->wtRegion.loopStart = gens[sfg_startloopAddrsOffset] + 32768 * gens[sfg_startloopAddrsCoarseOffset];
    pDLSRegion->wtRegion.loopEnd = gens[sfg_endloopAddrsOffset] + 32768 * gens[sfg_endloopAddrsCoarseOffset];
    
    // genValue is read as LE
    pDLSRegion->wtRegion.region.rangeLow = (EAS_U8)(gens[sfg_keyRange] & 0x7F);
    pDLSRegion->wtRegion.region.rangeHigh = (EAS_U8)((gens[sfg_keyRange] >> 8) & 0x7F);
    if (pDLSRegion->wtRegion.region.rangeLow > pDLSRegion->wtRegion.region.rangeHigh) {
        EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Invalid key range (%d, %d) at generator entry\n", pDLSRegion->wtRegion.region.rangeLow, pDLSRegion->wtRegion.region.rangeHigh);
    }
    pDLSRegion->velLow = (EAS_U8)(gens[sfg_velRange] & 0x7F);
    pDLSRegion->velHigh = (EAS_U8)((gens[sfg_velRange] >> 8) & 0x7F);
    if (pDLSRegion->velLow > pDLSRegion->velHigh) {
        EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Invalid velocity range (%d, %d) at generator entry\n", pDLSRegion->velLow, pDLSRegion->velHigh);
    }

    // Each 1 dB of attenuation set at the instrument or preset level should only attenuate the sound by 0.4 dB.
    // This is a quirk of the Sound Blaster hardware that should be emulated for compatibility with existing SoundFonts.
    pDLSRegion->wtRegion.gain = -gens[sfg_initialAttenuation] * 4 / 10;

    if (gens[sfg_sampleModes] == 1) { // loop forever
        *loopMode = 1; // loop forever
    } else if (gens[sfg_sampleModes] == 3) { // loop and release
        // TODO: eas does not support this
        EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: sampleModes 3 'loop and release' is not supported at generator entry\n");
        *loopMode = 1; // loop forever
    } else { // no loop
        *loopMode = 0;
    }

    if (gens[sfg_exclusiveClass] > 0x7f) {
        EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: exclusive class number %u is too large at generator entry\n", gens[sfg_exclusiveClass]);
        pDLSRegion->wtRegion.region.keyGroupAndFlags = REGION_FLAG_NON_SELF_EXCLUSIVE;
    } else {
        pDLSRegion->wtRegion.region.keyGroupAndFlags = ((gens[sfg_exclusiveClass] & 0x7f) << 8) | REGION_FLAG_NON_SELF_EXCLUSIVE;
    }

    // modulators
    for (EAS_U32 i = 0; i < modCount; i++) {
        const char* myOffset = (char*)pMods + (MOD_SIZE * (EAS_IPTR)i);
        const EAS_U16 modSrc = *(EAS_U16*)myOffset;
        const EAS_U16 modDest = *(EAS_I16*)(myOffset + 2);
        const EAS_I16 modAmount = *(EAS_U16*)(myOffset + 4);
        const EAS_U16 modControl = *(EAS_U16*)(myOffset + 6);
        const EAS_U16 modTrans = *(EAS_U16*)(myOffset + 8);

        //    Type      P  D   CC    Index
        // 15 ----- 10 09 08 | 07 06 ----- 00
        struct s_mod_src modSrcTag = ModSrc(modSrc);
        
        if (modSrcTag.index == sfm_noController) {
            continue;
        }

        if (modSrc == 0x00DB // (type=0, P=0, D=0, CC=1, index = 91)
            && modDest == sfg_reverbEffectsSend // CC91 -> reverbSend
        ) {
            if (modControl != 0) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: CC91 -> reverbSend does not support amount source 0x%04x\n", modControl);
            }
            if (modTrans != sft_Linear) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: CC91 -> reverbSend does not support transform 0x%04x\n", modTrans);
            }
            pDLSArt->cc91ToReverbSend += modAmount;
        } else if (modSrc == 0x00DD // (type=0, P=0, D=0, CC=1, index = 93)
            && modDest == sfg_chorusEffectsSend // CC93 -> chorusSend
        ) {
            if (modControl != 0) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: CC93 -> chorusSend does not support amount source 0x%04x\n", modControl);
            }
            if (modTrans != sft_Linear) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: CC93 -> chorusSend does not support transform 0x%04x\n", modTrans);
            }
            pDLSArt->cc93ToChorusSend += modAmount;
        } else if ((modSrc & 0xFF) == 0x02) { // (CC=0, index = 2) noteon velocity
            if (modSrcTag.polarity == EAS_TRUE) {
                goto invalid;
            }
            if (modControl != 0) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: note-on velocity does not support amount source 0x%04x\n", modControl);
            }
            if (modTrans != sft_Linear) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: note-on velocity does not support transform 0x%04x\n", modTrans);
            }
            switch (modDest) {
            case sfg_attackVolEnv:
                if (modSrcTag.type != sfct_Linear) {
                    EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: velToVolAttack does not support non-linear controller type 0x%02x\n", modTrans);
                }
                if (modSrcTag.direction == EAS_FALSE) { // 0 ~ 127
                    pDLSArt->eg1.velToAttack += modAmount;
                } else { // 127 ~ 0
                    pDLSArt->eg1.velToAttack -= modAmount;
                    pDLSArt->eg1.attackTime += modAmount;
                }
                break;
            case sfg_attackModEnv:
                if (modSrcTag.type != sfct_Linear) {
                    EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: velToModAttack does not support non-linear controller type 0x%02x\n", modTrans);
                }
                if (modSrcTag.direction == EAS_FALSE) { // 0 ~ 127
                    pDLSArt->eg2.velToAttack += modAmount;
                } else { // 127 ~ 0
                    pDLSArt->eg2.velToAttack -= modAmount;
                    pDLSArt->eg2.attackTime += modAmount;
                }
                break;
            case sfg_initialFilterFc:
                if (modSrcTag.type != sfct_Linear) {
                    EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: velToFc does not support non-linear controller type 0x%02x\n", modTrans);
                }
                if (modSrcTag.direction == EAS_FALSE) { // 0 ~ 127
                    pDLSArt->velToFc = modAmount;
                } else { // 127 ~ 0
                    pDLSArt->velToFc -= modAmount;
                    pDLSArt->filterCutoff += modAmount;
                }
                break;
            case sfg_initialAttenuation:
                if (modSrcTag.type != sfct_Concave) {
                    EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: velToAttenuation does not support non-concave controller type 0x%02x\n", modTrans);
                }
                if (abs(modAmount) < 5) { // 0.5 dB
                    pDLSArt->filterQandFlags &= ~FLAG_DLS_VELOCITY_SENSITIVE;
                } else {
                    if (modAmount != 960) { // 96 dB
                        EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: velToAttenuation does not support modAmount which is neither default nor non-zero: %d\n", (int)modAmount);
                    }
                    pDLSArt->filterQandFlags |= FLAG_DLS_VELOCITY_SENSITIVE;
                }
                break;
            default:
                goto invalid;
            }
        } else if (modSrc == 0x0003) { // (type=0, P=0, D=0, CC=0, index = 3) noteon key number
            if (modControl != 0) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: note-on key number does not support amount source 0x%04x\n", modControl);
            }
            if (modTrans != sft_Linear) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: note-on key number does not support transform 0x%04x\n", modTrans);
            }
            switch (modDest) {
            case sfg_initialFilterFc:
                pDLSArt->keyNumToFc += modAmount;
                break;
            default:
                goto invalid;
            }
        } else if (modSrc == 0x000D) { // (type=0, P=0, D=0, CC=0, index = 13) midi channel pressure
            if (modControl != 0) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: midi channel pressure does not support amount source 0x%04x\n", modControl);
            }
            if (modTrans != sft_Linear) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: midi channel pressure does not support transform 0x%04x\n", modTrans);
            }
            switch (modDest) {
            case sfg_vibLfoToPitch:
                pDLSArt->modLFOChanPressToPitch += modAmount;
                break;
            case sfg_modLfoToFilterFc:
                pDLSArt->modLFOChanPressToFc += modAmount;
                break;
            case sfg_modLfoToVolume:
                pDLSArt->modLFOChanPressToGain += modAmount;
                break;
            default:
                goto invalid;
            }
        } else if (modSrc == 0x0081) { // (type=0, P=0, D=0, CC=1, index = 1) CC1
            if (modControl != 0) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: CC1 does not support amount source 0x%04x\n", modControl);
            }
            if (modTrans != sft_Linear) {
                EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: CC1 does not support transform 0x%04x\n", modTrans);
            }
            switch (modDest) {
            case sfg_modLfoToFilterFc:
                pDLSArt->modLFOCC1ToFc += modAmount;
                break;
            case sfg_modLfoToVolume:
                pDLSArt->modLFOCC1ToGain += modAmount;
                break;
            case sfg_vibLfoToPitch:
                pDLSArt->vibLFOCC1ToPitch += modAmount;
                break;
            case sfg_modLfoToPitch:
                pDLSArt->modLFOCC1ToPitch += modAmount;
                break;
            default:
                goto invalid;
            }
        } else {
        invalid:
            EAS_Report(_EAS_SEVERITY_WARNING, "SF2Parser: Unsupported modulator 0x%04x*(0x%04x)--(%u)->0x%04x\n", modSrc, modControl, (unsigned)modTrans, modDest);
        }
    }

    if (gens[sfg_keynum] != -1) {
        pDLSArt->keyNumToPitch = 0;
        pDLSArt->tuning += gens[sfg_keynum] * 100;
    }

    if (gens[sfg_velocity] != -1 && (pDLSArt->filterQandFlags & FLAG_DLS_VELOCITY_SENSITIVE)) {
        pDLSRegion->wtRegion.gain += -960 * (127 - gens[sfg_velocity]) / 128;
        pDLSArt->filterQandFlags &= ~FLAG_DLS_VELOCITY_SENSITIVE;
    }

    return EAS_SUCCESS;
}
