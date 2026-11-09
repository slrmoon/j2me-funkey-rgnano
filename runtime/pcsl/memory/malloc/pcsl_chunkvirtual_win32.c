/*
 * PCSL chunk memory for the CLDC Java heap on Win32 (MinGW builds).
 *
 * The heap also holds JIT-compiled code, so it must be executable: the
 * malloc-based pcsl_chunkheap.c faults under DEP. Address space is
 * reserved once and committed/decommitted as the heap grows or shrinks;
 * the chunk never moves.
 */

#include <windows.h>
#include <pcsl_memory.h>
#include <pcsl_print.h>

typedef struct _ChunkInfo {
    void *addr;
    unsigned int size;          /* committed bytes */
    unsigned int reserved_size; /* reserved bytes */
    struct _ChunkInfo *next;
} ChunkInfo;

static ChunkInfo *chunks = NULL;

static unsigned int page_align_up(unsigned int size) {
    SYSTEM_INFO si;
    unsigned int page;
    GetSystemInfo(&si);
    page = si.dwPageSize;
    return (size + (page - 1)) & ~(page - 1);
}

static ChunkInfo *get_chunk_info(void *chunk_ptr) {
    ChunkInfo *ci;
    for (ci = chunks; ci; ci = ci->next) {
        if (ci->addr == chunk_ptr) {
            return ci;
        }
    }
    return NULL;
}

void *pcsl_mem_allocate_chunk(unsigned int initial_size,
                              unsigned int max_size,
                              unsigned int alignment) {
    void *chunk;
    ChunkInfo *ci;
    unsigned int committed;

    (void)alignment; /* VirtualAlloc returns 64K-aligned addresses */
    max_size = page_align_up(max_size);
    committed = page_align_up(initial_size);
    if (committed > max_size) {
        committed = max_size;
    }

    chunk = VirtualAlloc(NULL, max_size, MEM_RESERVE, PAGE_NOACCESS);
    if (chunk == NULL) {
        return NULL;
    }
    if (committed > 0 &&
        VirtualAlloc(chunk, committed, MEM_COMMIT,
                     PAGE_EXECUTE_READWRITE) == NULL) {
        VirtualFree(chunk, 0, MEM_RELEASE);
        return NULL;
    }

    ci = (ChunkInfo *)pcsl_mem_malloc(sizeof(ChunkInfo));
    if (ci == NULL) {
        VirtualFree(chunk, 0, MEM_RELEASE);
        return NULL;
    }
    ci->addr = chunk;
    ci->size = committed;
    ci->reserved_size = max_size;
    ci->next = chunks;
    chunks = ci;
    return chunk;
}

/* Expand or shrink a chunk returned by pcsl_mem_allocate_chunk().
 * Returns 0 if it cannot expand (shrinking always succeeds), otherwise
 * the old size. */
unsigned int pcsl_mem_adjust_chunk(void *chunk_ptr, unsigned int new_size) {
    ChunkInfo *ci = get_chunk_info(chunk_ptr);
    unsigned int old_size;

    if (ci == NULL) {
        return 0;
    }
    old_size = ci->size;
    new_size = page_align_up(new_size);
    if (new_size > ci->reserved_size) {
        return 0;
    }
    if (new_size > old_size) {
        if (VirtualAlloc((char *)chunk_ptr + old_size, new_size - old_size,
                         MEM_COMMIT, PAGE_EXECUTE_READWRITE) == NULL) {
            return 0;
        }
    } else if (new_size < old_size) {
        VirtualFree((char *)chunk_ptr + new_size, old_size - new_size,
                    MEM_DECOMMIT);
    }
    ci->size = new_size;
    return old_size;
}

void pcsl_mem_free_chunk(void *chunk_ptr) {
    ChunkInfo *ci;
    ChunkInfo *last = NULL;
    for (ci = chunks; ci; last = ci, ci = ci->next) {
        if (ci->addr == chunk_ptr) {
            if (last == NULL) {
                chunks = ci->next;
            } else {
                last->next = ci->next;
            }
            VirtualFree(ci->addr, 0, MEM_RELEASE);
            pcsl_mem_free((void *)ci);
            return;
        }
    }
}
