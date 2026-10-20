#include "eas_data.h"
#include "eas_parser.h"
#include "eas_mdls.h"
#include "eas_smf.h"
#include "eas_host.h"
#include "eas_report.h"
#include "eas_vm_protos.h"

#define RIFF_IDENTIFIER          0x52494646  // 'RIFF'
#define RMID_IDENTIFIER          0x524d4944  // 'RMID'
#define DATA_IDENTIFIER          0x64617461  // 'data'
#define LIST_IDENTIFIER          0x4c495354  // 'LIST'

typedef struct S_RMID_DATA {
    EAS_FILE_HANDLE fileHandle;
    EAS_U32 rmidSize;

    EAS_I32 smfOffest;
    EAS_U32 smfSize;
    EAS_VOID_PTR smfData;
    
    EAS_I32 dlsOffset;
    EAS_U32 dlsSize;
    S_DLS* dlsData;
} S_RMID_DATA;

static EAS_RESULT RMID_CheckFileType(S_EAS_DATA *pEASData, EAS_FILE_HANDLE fileHandle, EAS_VOID_PTR *ppHandle, EAS_I32 offset);
static EAS_RESULT RMID_Prepare(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData);
static EAS_RESULT RMID_Time(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData, EAS_U32 *pTime);
static EAS_RESULT RMID_Event(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData, EAS_INT parserMode);
static EAS_RESULT RMID_State(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData, EAS_STATE *pState);
static EAS_RESULT RMID_Close(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData);
static EAS_RESULT RMID_Reset(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData);
static EAS_RESULT RMID_Pause(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData);
static EAS_RESULT RMID_Resume(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData);
static EAS_RESULT RMID_SetData(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData, EAS_I32 param, EAS_IPTR value);
static EAS_RESULT RMID_GetData(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData, EAS_I32 param, EAS_IPTR *pValue);

const S_FILE_PARSER_INTERFACE EAS_RMID_Parser = {
    RMID_CheckFileType,
    RMID_Prepare,
    RMID_Time,
    RMID_Event,
    RMID_State,
    RMID_Close,
    RMID_Reset,
    RMID_Pause,
    RMID_Resume,
    NULL, // No Locate
    RMID_SetData,
    RMID_GetData,
    NULL  // No GetMetaData
};

static EAS_RESULT RMID_ReadChunk(S_EAS_DATA *pEASData, S_RMID_DATA *pRMIDData, EAS_U32* chunkType, EAS_U32* dataSize, EAS_I32* dataOffset)
{
    EAS_RESULT result;
    
    result = EAS_HWGetDWord(pEASData->hwInstData, pRMIDData->fileHandle, chunkType, EAS_TRUE);
    if (result != EAS_SUCCESS) {
        return result;
    }

    result = EAS_HWGetDWord(pEASData->hwInstData, pRMIDData->fileHandle, dataSize, EAS_FALSE);
    if (result != EAS_SUCCESS) {
        return result;
    }

    result = EAS_HWFilePos(pEASData->hwInstData, pRMIDData->fileHandle, dataOffset);
    if (result != EAS_SUCCESS) {
        return result;
    }

    return EAS_SUCCESS;
}

static EAS_RESULT RMID_CheckFileType(S_EAS_DATA *pEASData, EAS_FILE_HANDLE fileHandle, EAS_VOID_PTR *ppHandle, EAS_I32 offset)
{
    EAS_U32 temp;
    EAS_RESULT result;
    S_RMID_DATA* pRMIDData;

    *ppHandle = NULL;

    result = EAS_HWGetDWord(pEASData->hwInstData, fileHandle, &temp, EAS_TRUE);
    if (result != EAS_SUCCESS) {
        return result;
    }
    if (temp != RIFF_IDENTIFIER) {
        EAS_Report(_EAS_SEVERITY_DETAIL, "RMID_CheckFileType: Not a RIFF file (0x%08x)\n", temp);
        return EAS_SUCCESS;
    }

    if (pEASData->staticMemoryModel) {
        EAS_Report(_EAS_SEVERITY_ERROR, "RMID_CheckFileType: Static memory model not supported\n");
        return EAS_SUCCESS;
    }

    pRMIDData = EAS_HWMalloc(pEASData->hwInstData, sizeof(S_RMID_DATA));
    if (!pRMIDData) {
        EAS_Report(_EAS_SEVERITY_ERROR, "RMID_CheckFileType: HWMalloc failed\n");
        return EAS_ERROR_MALLOC_FAILED;
    }

    EAS_HWMemSet(pRMIDData, 0, sizeof(S_RMID_DATA));
    pRMIDData->fileHandle = fileHandle;

    result = EAS_HWGetDWord(pEASData->hwInstData, fileHandle, &pRMIDData->rmidSize, EAS_FALSE);
    if (result != EAS_SUCCESS) {
        goto error_cleanup;
    }

    result = EAS_HWGetDWord(pEASData->hwInstData, fileHandle, &temp, EAS_TRUE);
    if (result != EAS_SUCCESS) {
        goto error_cleanup;
    }
    if (temp != RMID_IDENTIFIER) {
        EAS_Report(_EAS_SEVERITY_DETAIL, "RMID_CheckFileType: Not a RMID file (0x%08x)\n", temp);
        // Not a RMID file, return EAS_SUCCESS here
        goto error_cleanup;
    }

    EAS_I32 remainingBytes = pRMIDData->rmidSize - 4; // 4 bytes for the RMID identifier

    while (remainingBytes > 1) {
        EAS_U32 chunkType, dataSize;
        EAS_I32 dataOffset;
        // Read chunks
        result = RMID_ReadChunk(pEASData, pRMIDData, &chunkType, &dataSize, &dataOffset);
        if (result == EAS_EOF) {
            // End of file reached
            break;
        }
        if (result != EAS_SUCCESS) {
            EAS_Report(_EAS_SEVERITY_ERROR, "RMID_CheckFileType: Failed to read chunk: %ld\n", result);
            goto error_cleanup;
        }

        if (chunkType == DATA_IDENTIFIER) {
            // Found SMF
            pRMIDData->smfOffest = dataOffset;
            pRMIDData->smfSize = dataSize;
        } else if (chunkType == RIFF_IDENTIFIER) {
            // Found DLS
            // we have read RIFF header of DLS
            pRMIDData->dlsOffset = dataOffset - 8;
            pRMIDData->dlsSize = dataSize + 8;
        } else if (chunkType == LIST_IDENTIFIER) {
            // it should be a INFO chunk, we don't care
        } else {
            EAS_Report(_EAS_SEVERITY_INFO, "RMID_CheckFileType: Unknown chunk type (0x%08x)\n", chunkType);
        }

        result = EAS_HWFileSeekOfs(pEASData->hwInstData, fileHandle, dataSize); // Skip to the end of the chunk
        if (result != EAS_SUCCESS) {
            EAS_Report(_EAS_SEVERITY_ERROR, "RMID_CheckFileType: Failed to seek in file: %ld\n", result);
            goto error_cleanup;
        }

        result = EAS_HWFilePos(pEASData->hwInstData, fileHandle, &temp);
        
        if (result != EAS_SUCCESS) {
            goto error_cleanup;
        }
        if (temp % 2 != 0) {
            // If the file size is odd, we need to skip one byte to align to even
            result = EAS_HWFileSeekOfs(pEASData->hwInstData, fileHandle, 1);
            if (result == EAS_EOF) {
                break;
            }
            if (result != EAS_SUCCESS) {
                goto error_cleanup;
            }
            remainingBytes -= 1;
        }

        remainingBytes -= (8 + dataSize); // 8 bytes for chunk type and size
    }

    if (pRMIDData->smfOffest <= 0 || pRMIDData->smfSize == 0) {
        EAS_Report(_EAS_SEVERITY_ERROR, "RMID_CheckFileType: No SMF data found\n");
        result = EAS_ERROR_FILE_FORMAT;
        goto error_cleanup;
    }

    result = SMF_CheckFileType(pEASData, pRMIDData->fileHandle, &pRMIDData->smfData, pRMIDData->smfOffest);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "RMID_Prepare: Failed to check SMF file type: %ld\n", result);
        goto error_cleanup;
    }
    if (pRMIDData->smfData == NULL) {
        EAS_Report(_EAS_SEVERITY_ERROR, "RMID_Prepare: Not a valid SMF file\n");
        result = EAS_ERROR_FILE_FORMAT;
        goto error_cleanup;
    }

    *ppHandle = pRMIDData;
    return EAS_SUCCESS;
    
error_cleanup:
    EAS_HWFree(pEASData->hwInstData, pRMIDData);
    return result;
}

static EAS_RESULT RMID_Prepare(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData)
{
    S_RMID_DATA* pRMIDData = (S_RMID_DATA*)pInstData;
    EAS_RESULT result;

    // Check if we have a valid SMF offset
    if (pRMIDData->smfOffest <= 0 || pRMIDData->smfSize == 0) {
        EAS_Report(_EAS_SEVERITY_ERROR, "RMID_Prepare: Invalid SMF offset or size\n");
        return EAS_ERROR_FILE_FORMAT;
    }

    // Prepare SMF
    result = SMF_Prepare(pEASData, pRMIDData->smfData);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "RMID_Prepare: Failed to prepare SMF: %ld\n", result);
        return result;
    }

    if (pRMIDData->dlsOffset == 0) {
        return EAS_SUCCESS; // No DLS
    }

    // Parse DLS
    result = DLSParser(pEASData->hwInstData, pRMIDData->fileHandle, pRMIDData->dlsOffset, &pRMIDData->dlsData);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "RMID_Prepare: Failed to parse DLS data: %ld, proceed without DLS\n", result);
        return EAS_SUCCESS; // Proceed without DLS
    }

    result = VMSetDLSLib(((S_SMF_DATA*)pRMIDData->smfData)->pSynth, pRMIDData->dlsData);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_ERROR, "RMID_Prepare: Failed to set DLS library: %ld\n", result);
        return result;
    }
    DLSAddRef(pRMIDData->dlsData);
    VMInitializeAllChannels(pEASData->pVoiceMgr, ((S_SMF_DATA*)pRMIDData->smfData)->pSynth);

    return EAS_SUCCESS;
}

static EAS_RESULT RMID_Time(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData, EAS_U32 *pTime)
{
    S_RMID_DATA* pRMIDData = (S_RMID_DATA*)pInstData;
    return SMF_Time(pEASData, pRMIDData->smfData, pTime);
}

static EAS_RESULT RMID_Event(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData, EAS_INT parserMode)
{
    S_RMID_DATA* pRMIDData = (S_RMID_DATA*)pInstData;
    return SMF_Event(pEASData, pRMIDData->smfData, parserMode);
}

static EAS_RESULT RMID_State(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData, EAS_STATE *pState)
{
    S_RMID_DATA* pRMIDData = (S_RMID_DATA*)pInstData;
    return SMF_State(pEASData, pRMIDData->smfData, pState);
}

static EAS_RESULT RMID_Close(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData)
{
    EAS_RESULT result;
    S_RMID_DATA* pRMIDData = (S_RMID_DATA*)pInstData;

    // This should close file handle
    result = SMF_Close(pEASData, pRMIDData->smfData);
    if (result != EAS_SUCCESS) {
        EAS_Report(_EAS_SEVERITY_WARNING, "RMID_Close: Failed to close SMF data: %ld\n", result);
    }
    pRMIDData->smfData = NULL;

    if (pRMIDData->dlsData) {
        DLSCleanup(pEASData->hwInstData, pRMIDData->dlsData);
        pRMIDData->dlsData = NULL;
    }

    EAS_HWFree(pEASData->hwInstData, pRMIDData);
    return EAS_SUCCESS;
}

static EAS_RESULT RMID_Reset(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData)
{
    S_RMID_DATA* pRMIDData = (S_RMID_DATA*)pInstData;
    return SMF_Reset(pEASData, pRMIDData->smfData);
}

static EAS_RESULT RMID_Pause(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData)
{
    S_RMID_DATA* pRMIDData = (S_RMID_DATA*)pInstData;
    return SMF_Pause(pEASData, pRMIDData->smfData);
}

static EAS_RESULT RMID_Resume(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData)
{
    S_RMID_DATA* pRMIDData = (S_RMID_DATA*)pInstData;
    return SMF_Resume(pEASData, pRMIDData->smfData);
}

static EAS_RESULT RMID_SetData(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData, EAS_I32 param, EAS_IPTR value)
{
    S_RMID_DATA* pRMIDData = (S_RMID_DATA*)pInstData;
    return SMF_SetData(pEASData, pRMIDData->smfData, param, value);
}

static EAS_RESULT RMID_GetData(S_EAS_DATA *pEASData, EAS_VOID_PTR pInstData, EAS_I32 param, EAS_IPTR *pValue)
{
    S_RMID_DATA* pRMIDData = (S_RMID_DATA*)pInstData;
    return SMF_GetData(pEASData, pRMIDData->smfData, param, pValue);
}
