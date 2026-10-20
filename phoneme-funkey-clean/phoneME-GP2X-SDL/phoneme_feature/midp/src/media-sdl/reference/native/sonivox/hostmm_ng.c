// hostmm_ng.c
// This implements EAS Host Wrapper, using C runtime library, with better performance

#include "eas_options.h"
#include "eas_host.h"

#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else // Unix like
#include <unistd.h>
#endif

/* Only for debugging LED, vibrate, and backlight functions */
#include "eas_report.h"

#if defined(__GNUC__) || defined(__clang__)
#define bswap16 __builtin_bswap16
#define bswap32 __builtin_bswap32
#elif __has_include(<byteswap.h>)
#include <byteswap.h>
#define bswap16 bswap_16
#define bswap32 bswap_32
#elif defined(_MSC_VER)
#define bswap16 _byteswap_ushort
#define bswap32 _byteswap_ulong
#endif

const EAS_BOOL O32_BIG_ENDIAN = 
#ifdef EAS_BIG_ENDIAN
    EAS_TRUE;
#else
    EAS_FALSE;
#endif

typedef struct eas_hw_file_tag {
    void* handle;
    EAS_BOOL own;

    // legacy interface for compatibility
    int pos;
    int (*readAt)(void *handle, void *buf, int offset, int size);
    int (*size)(void *handle);
} EAS_HW_FILE;

#if !defined(__linux__) && !defined(__APPLE__) && !defined(_WIN32)
#define INTERNAL_READAT
int ReadAt(void *handle, void *buf, int offset, int size)
{
    int ret;

    ret = fseek((FILE *) handle, offset, SEEK_SET);
    if (ret < 0) return 0;

    return fread(buf, 1, size, (FILE *) handle);
}

int Size(void *handle) {
    int ret;

    ret = fseek((FILE *) handle, 0, SEEK_END);
    if (ret < 0) return ret;

    return ftell((FILE *) handle);
}
#endif

EAS_RESULT EAS_HWInit(EAS_HW_DATA_HANDLE* pHWInstData)
{
    return EAS_SUCCESS;
}

EAS_RESULT EAS_HWShutdown(EAS_HW_DATA_HANDLE hwInstData)
{
    return EAS_SUCCESS;
}

void* EAS_HWMalloc(EAS_HW_DATA_HANDLE hwInstData, EAS_I32 size)
{
    return malloc((size_t)size);
}

void EAS_HWFree(EAS_HW_DATA_HANDLE hwInstData, void* p)
{
    free(p);
}

void* EAS_HWMemCpy(void* dest, const void* src, EAS_I32 amount)
{
    return memcpy(dest, src, (size_t)amount);
}

void* EAS_HWMemSet(void* dest, int val, EAS_I32 amount)
{
    return memset(dest, val, (size_t)amount);
}

EAS_I32 EAS_HWMemCmp(const void* s1, const void* s2, EAS_I32 amount)
{
    return (EAS_I32)memcmp(s1, s2, (size_t)amount);
}

EAS_RESULT EAS_HWOpenFile(EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_LOCATOR locator, EAS_FILE_HANDLE* pFile, EAS_FILE_MODE mode)
{
    if (pFile == NULL) {
        return EAS_ERROR_INVALID_PARAMETER;
    }
    if (!(
        (locator->readAt == NULL && locator->size == NULL)
        || (locator->readAt != NULL && locator->size != NULL)
    )) {
        EAS_Report(_EAS_SEVERITY_ERROR, "EAS_HWOpenFile: Must specify both readAt and size or neither\n");
        return EAS_ERROR_INVALID_PARAMETER;
    }

    *pFile = malloc(sizeof(EAS_HW_FILE));
    (*pFile)->handle = locator->handle;
    (*pFile)->own = EAS_FALSE;
    (*pFile)->readAt = locator->readAt;
    (*pFile)->size = locator->size;

#if defined(INTERNAL_READAT)
    if ((*pFile)->readAt == NULL){
        (*pFile)->readAt = ReadAt;
        (*pFile)->size = Size;
    }
#endif

    return EAS_SUCCESS;
}

EAS_RESULT EAS_HWReadFile(EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void* pBuffer, EAS_I32 n, EAS_I32* pBytesRead)
{
    /* make sure we have a valid handle */
    if (file->handle == NULL) {
        return EAS_ERROR_INVALID_HANDLE;
    }

    if (file->readAt != NULL) {
        int count = file->readAt(file->handle, pBuffer, file->pos, n);
        file->pos += count;
        if (pBytesRead != NULL) {
            *pBytesRead = count;
        }
        if (count < n) {
            return EAS_EOF;
        }
        return EAS_SUCCESS;
    }

    size_t count = fread(pBuffer, 1, n, file->handle);
    if (pBytesRead != NULL) {
        *pBytesRead = count;
    }

    if (feof((FILE *) file->handle)) {
        return EAS_EOF;
    }
    if (ferror((FILE *) file->handle)) {
        return EAS_ERROR_FILE_READ_FAILED;
    }
    return EAS_SUCCESS;
}

EAS_RESULT EAS_HWGetByte(EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void* p)
{
    return EAS_HWReadFile(hwInstData, file, p, 1, NULL);
}

EAS_RESULT EAS_HWGetWord(EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void* p, EAS_BOOL msbFirst)
{
    EAS_U16 word;
    *((EAS_U16*)p) = 0;
    EAS_RESULT result = EAS_HWReadFile(hwInstData, file, &word, sizeof(word), NULL);
    if (result != EAS_SUCCESS) {
        return result;
    }

    if (msbFirst ^ O32_BIG_ENDIAN) {
        *((EAS_U16*)p) = bswap16(word);
    } else {
        *((EAS_U16*)p) = word;
    }

    return EAS_SUCCESS;
}

/*lint -esym(715, hwInstData) hwInstData available for customer use */
EAS_RESULT EAS_HWGetDWord(EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, void* p, EAS_BOOL msbFirst)
{
    uint32_t dword; // EAS_U32 is not uint32_t
    *((EAS_U32 *) p) = 0;
    EAS_RESULT result = EAS_HWReadFile(hwInstData, file, &dword, sizeof(dword), NULL);
    if (result != EAS_SUCCESS) {
        return result;
    }

    if (msbFirst ^ O32_BIG_ENDIAN) {
        *((EAS_U32 *) p) = bswap32(dword);
    } else {
        *((EAS_U32 *) p) = dword;
    }

    return EAS_SUCCESS;
}

EAS_RESULT EAS_HWFilePos(EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32* pPosition)
{
    /* make sure we have a valid handle */
    if (file->handle == NULL) {
        return EAS_ERROR_INVALID_HANDLE;
    }

    if (file->readAt != NULL) {
        if (pPosition != NULL) {
            *pPosition = file->pos;
        }
        return EAS_SUCCESS;
    }

    long pos = ftell(file->handle);
    if (pPosition != NULL) {
        *pPosition = pos;
    }
    return EAS_SUCCESS;
} /* end EAS_HWFilePos */

EAS_RESULT EAS_HWFileSeek(EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 position)
{
    /* make sure we have a valid handle */
    if (file->handle == NULL) {
        return EAS_ERROR_INVALID_HANDLE;
    }

    if (file->readAt != NULL) {
        file->pos = position;
        return EAS_SUCCESS;
    }

    /* validate new position */
    if (fseek(file->handle, position, SEEK_SET) != 0) {
        return EAS_ERROR_FILE_SEEK;
    }

    return EAS_SUCCESS;
}

EAS_RESULT EAS_HWFileSeekOfs(EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_I32 position)
{
    /* make sure we have a valid handle */
    if (file->handle == NULL) {
        return EAS_ERROR_INVALID_HANDLE;
    }

    if (file->readAt != NULL) {
        file->pos += position;
        return EAS_SUCCESS;
    }

    if (fseek(file->handle, position, SEEK_CUR) != 0) {
        return EAS_ERROR_FILE_SEEK;
    }

    return EAS_SUCCESS;
}

EAS_RESULT EAS_HWDupHandle(EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file, EAS_FILE_HANDLE* pDupFile)
{
    *pDupFile = NULL;

    if (file->readAt != NULL) {
        *pDupFile = malloc(sizeof(EAS_HW_FILE));
        (*pDupFile)->handle = file->handle;
        (*pDupFile)->own = EAS_FALSE;
        (*pDupFile)->readAt = file->readAt;
        (*pDupFile)->size = file->size;
        (*pDupFile)->pos = file->pos;
        return EAS_SUCCESS;
    }

#if defined(__linux__) || defined(__APPLE__)
    char filePath[PATH_MAX];

#if defined(__linux__)
    char linkPath[PATH_MAX];
    snprintf(linkPath, PATH_MAX, "/proc/self/fd/%d", fileno(file->handle));

    ssize_t len = readlink(linkPath, filePath, PATH_MAX);
    if (len == -1) {
        return EAS_ERROR_INVALID_HANDLE;
    }
    filePath[len] = '\0';
#else // __APPLE__
    if (fcntl(fileno(file->handle), F_GETPATH, filePath) == -1) {
        return EAS_ERROR_INVALID_HANDLE;
    }
#endif

    EAS_HW_FILE *new_file = malloc(sizeof(EAS_HW_FILE));
    memset(new_file, 0, sizeof(EAS_HW_FILE));
    new_file->handle = fopen(filePath, "rb"); // always dup as rb, should be okay
    new_file->own = EAS_TRUE;
    if (new_file->handle == NULL) {
        free(new_file);
        return EAS_ERROR_INVALID_HANDLE;
    }

    long pos = ftell(file->handle);
    if (pos == -1) {
        fclose(new_file->handle);
        free(new_file);
        return EAS_ERROR_FILE_POS;
    }
    if (fseek(new_file->handle, pos, SEEK_SET) != 0) {
        fclose(new_file->handle);
        free(new_file);
        return EAS_ERROR_FILE_SEEK;
    }

    *pDupFile = new_file;
    return EAS_SUCCESS;
#elif defined(_WIN32)
    char pathName[MAX_PATH];
    int fd = _fileno(file->handle);
    if (fd == -1) {
        return EAS_ERROR_INVALID_HANDLE;
    }
    HANDLE hnd = (HANDLE)_get_osfhandle(fd);
    if (hnd == INVALID_HANDLE_VALUE) {
        return EAS_ERROR_INVALID_HANDLE;
    }
    
    if (GetFinalPathNameByHandle(hnd, pathName, MAX_PATH, 0) == 0) {
        return EAS_ERROR_NOT_VALID_IN_THIS_STATE;
    }

    EAS_HW_FILE* new_file = malloc(sizeof(EAS_HW_FILE));
    memset(new_file, 0, sizeof(EAS_HW_FILE));
    new_file->handle = fopen(pathName, "rb"); // always dup as rb, should be okay   
    new_file->own = EAS_TRUE;
    if (new_file->handle == NULL) {
        free(new_file);
        return EAS_ERROR_INVALID_HANDLE;
    }
    
    long pos = ftell(file->handle);
    if (pos == -1) {
        fclose(new_file->handle);
        free(new_file);
        return EAS_ERROR_FILE_POS;
    }
    if (fseek(new_file->handle, pos, SEEK_SET) != 0) {
        fclose(new_file->handle);
        free(new_file);
        return EAS_ERROR_FILE_SEEK;
    }

    *pDupFile = new_file;
    return EAS_SUCCESS;
#endif
}

EAS_RESULT EAS_HWCloseFile(EAS_HW_DATA_HANDLE hwInstData, EAS_FILE_HANDLE file)
{
    /* make sure we have a valid handle */
    if (file->handle == NULL) {
        return EAS_ERROR_INVALID_HANDLE;
    }

    if (file->readAt != NULL) {
        return EAS_SUCCESS;
    }

    if (file->own) {
        if (fclose(file->handle) != 0) {
            return EAS_ERROR_INVALID_HANDLE;
        }
    }
    
    free(file);

    return EAS_SUCCESS;
}

EAS_RESULT EAS_HWVibrate(EAS_HW_DATA_HANDLE hwInstData, EAS_BOOL state)
{
    EAS_ReportEx(_EAS_SEVERITY_NOFILTER, 0x1a54b6e8, 0x00000001, state);
    return EAS_SUCCESS;
} /* end EAS_HWVibrate */

EAS_RESULT EAS_HWLED(EAS_HW_DATA_HANDLE hwInstData, EAS_BOOL state)
{
    EAS_ReportEx(_EAS_SEVERITY_NOFILTER, 0x1a54b6e8, 0x00000002, state);
    return EAS_SUCCESS;
}

EAS_RESULT EAS_HWBackLight(EAS_HW_DATA_HANDLE hwInstData, EAS_BOOL state)
{
    EAS_ReportEx(_EAS_SEVERITY_NOFILTER, 0x1a54b6e8, 0x00000003, state);
    return EAS_SUCCESS;
}

EAS_BOOL EAS_HWYield(EAS_HW_DATA_HANDLE hwInstData)
{
    // just let it run
    return EAS_FALSE;
}
