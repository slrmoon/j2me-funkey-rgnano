/*
 * FunKeyJ2ME.exe: Windows launcher of the desktop runtime (bin\runMidlet.exe).
 *
 *   FunKeyJ2ME.exe               settings window: game list, display,
 *                                controls (PC key map), sound and logs
 *   FunKeyJ2ME.exe game.jar      start the game right away (also: drop a
 *                                .jar / .jam on the exe)
 *
 * Settings live in settings.ini next to the exe; a game can have its own
 * full copy in data\games\<game>.ini ("Own settings for the selected game").
 * Environment variables of the same name still win (SCALE, LCD, MODE, ...).
 *
 * MIDP: the main class comes from MIDlet-1 of a .jad next to the jar or of
 * the manifest. DoJa (i-appli): a .jam next to the jar (game.jam, or the only
 * .jam of the folder whose AppClass is in the jar), scratchpad game.sp.
 *
 * The VM opens files through the ANSI C library, so every path it gets is
 * ASCII: 8.3 short names where Windows has them, otherwise copies in the
 * data folder (the scratchpad is copied back when the game ends).
 *
 * Build: see package.sh (windres launcher.rc; gcc -municode -mwindows
 *        launcher.c -lz -lcomctl32 -lcomdlg32 -lshlwapi -lole32 -lshell32)
 */
#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define _WIN32_IE 0x0600
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <zlib.h>
#include "launcher_res.h"

#define APP_TITLE L"FunKey J2ME"
#define WBUF 2048
#define WM_APP_GAME_ENDED (WM_APP + 1)

/* ================================================================ utils */

static void fail(const wchar_t *fmt, ...) {
    wchar_t msg[4096];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(msg, 4095, fmt, ap);
    msg[4095] = 0;
    va_end(ap);
    MessageBoxW(NULL, msg, APP_TITLE, MB_OK | MB_ICONERROR);
}

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (p == NULL) {
        MessageBoxW(NULL, L"Out of memory", APP_TITLE, MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }
    return p;
}

static int is_ascii_w(const wchar_t *s) {
    for (; *s; s++) if (*s < 0x20 || *s > 0x7e) return 0;
    return 1;
}

static int is_ascii_a(const char *s) {
    for (; *s; s++) if ((unsigned char)*s < 0x20 || (unsigned char)*s > 0x7e) return 0;
    return 1;
}

static char *narrow(const wchar_t *w) {        /* ASCII only */
    size_t n = wcslen(w), i;
    char *a = xmalloc(n + 1);
    for (i = 0; i <= n; i++) a[i] = (char)w[i];
    return a;
}

static wchar_t *widen_cp(const char *s, UINT cp) {
    int n = MultiByteToWideChar(cp, 0, s, -1, NULL, 0);
    wchar_t *w = xmalloc((size_t)(n > 0 ? n : 1) * sizeof(wchar_t));
    if (n <= 0 || !MultiByteToWideChar(cp, 0, s, -1, w, n)) w[0] = 0;
    return w;
}

static wchar_t *widen_utf8(const char *s) { return widen_cp(s, CP_UTF8); }

static char *utf8_of(const wchar_t *w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    char *a = xmalloc((size_t)(n > 0 ? n : 1));
    if (n <= 0 || !WideCharToMultiByte(CP_UTF8, 0, w, -1, a, n, NULL, NULL)) a[0] = 0;
    return a;
}

static int file_exists(const wchar_t *p) {
    DWORD a = GetFileAttributesW(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static int dir_exists(const wchar_t *p) {
    DWORD a = GetFileAttributesW(p);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

static void mkdirs(const wchar_t *path) {
    wchar_t buf[WBUF];
    size_t i;
    wcsncpy(buf, path, WBUF - 1);
    buf[WBUF - 1] = 0;
    for (i = 3; buf[i]; i++) {
        if (buf[i] == L'\\' || buf[i] == L'/') {
            wchar_t c = buf[i];
            buf[i] = 0;
            CreateDirectoryW(buf, NULL);
            buf[i] = c;
        }
    }
    CreateDirectoryW(buf, NULL);
}

static int read_file(const wchar_t *path, unsigned char **data, DWORD *size) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL, OPEN_EXISTING, 0, NULL);
    DWORD sz, got = 0;
    if (h == INVALID_HANDLE_VALUE) return 0;
    sz = GetFileSize(h, NULL);
    if (sz == INVALID_FILE_SIZE || sz > 256u * 1024 * 1024) { CloseHandle(h); return 0; }
    *data = xmalloc(sz + 1);
    if (!ReadFile(h, *data, sz, &got, NULL) || got != sz) {
        CloseHandle(h); free(*data); return 0;
    }
    (*data)[sz] = 0;
    *size = sz;
    CloseHandle(h);
    return 1;
}

static int write_file(const wchar_t *path, const void *data, DWORD size) {
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    DWORD put = 0;
    int ok;
    if (h == INVALID_HANDLE_VALUE) return 0;
    ok = WriteFile(h, data, size, &put, NULL) && put == size;
    CloseHandle(h);
    return ok;
}

/* a path the VM can open: the path itself, its 8.3 short name, or NULL */
static char *ascii_alias(const wchar_t *path) {
    wchar_t sh[WBUF];
    DWORD n;
    if (is_ascii_w(path)) return narrow(path);
    n = GetShortPathNameW(path, sh, WBUF);
    if (n > 0 && n < WBUF && is_ascii_w(sh)) return narrow(sh);
    return NULL;
}

/* dst = src: an existing identical copy (in use by a running game) is kept */
static int link_or_copy(const wchar_t *src, const wchar_t *dst) {
    WIN32_FILE_ATTRIBUTE_DATA a, b;
    if (GetFileAttributesExW(src, GetFileExInfoStandard, &a) &&
        GetFileAttributesExW(dst, GetFileExInfoStandard, &b) &&
        a.nFileSizeLow == b.nFileSizeLow && a.nFileSizeHigh == b.nFileSizeHigh &&
        CompareFileTime(&a.ftLastWriteTime, &b.ftLastWriteTime) == 0) {
        return 1;
    }
    DeleteFileW(dst);
    if (CreateHardLinkW(dst, src, NULL)) return 1;
    return CopyFileW(src, dst, FALSE);
}

static void dirname_w(const wchar_t *p, wchar_t *out) {
    wchar_t *s;
    wcsncpy(out, p, WBUF - 1);
    out[WBUF - 1] = 0;
    s = wcsrchr(out, L'\\');
    if (wcsrchr(out, L'/') > s) s = wcsrchr(out, L'/');
    if (s) *s = 0; else wcscpy(out, L".");
}

static const wchar_t *basename_w(const wchar_t *p) {
    const wchar_t *a = wcsrchr(p, L'\\'), *b = wcsrchr(p, L'/');
    if (b > a) a = b;
    return a ? a + 1 : p;
}

/* ================================================================== jar */

typedef struct {
    unsigned char *data;
    DWORD size;
    DWORD cd_offset;
    unsigned entries;
} Zip;

static unsigned u16(const unsigned char *p) { return p[0] | (p[1] << 8); }
static DWORD u32(const unsigned char *p) {
    return (DWORD)p[0] | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24);
}

static int zip_open(Zip *z, const wchar_t *path) {
    long i;
    memset(z, 0, sizeof(*z));
    if (!read_file(path, &z->data, &z->size)) return 0;
    for (i = (long)z->size - 22; i >= 0 && i >= (long)z->size - 22 - 65535; i--) {
        if (u32(z->data + i) == 0x06054b50) {
            z->entries = u16(z->data + i + 10);
            z->cd_offset = u32(z->data + i + 16);
            if (z->cd_offset < z->size) return 1;
            break;
        }
    }
    free(z->data);
    z->data = NULL;
    return 0;
}

static void zip_close(Zip *z) {
    free(z->data);
    z->data = NULL;
}

static unsigned char *zip_read(Zip *z, const char *name, int nocase, DWORD *outlen) {
    DWORD p = z->cd_offset;
    size_t nlen = strlen(name);
    unsigned i;
    for (i = 0; i < z->entries && p + 46 <= z->size; i++) {
        const unsigned char *c = z->data + p;
        unsigned method, fn, ex, cm;
        DWORD csize, usize, lho;
        if (u32(c) != 0x02014b50) break;
        method = u16(c + 10);
        csize = u32(c + 20);
        usize = u32(c + 24);
        fn = u16(c + 28); ex = u16(c + 30); cm = u16(c + 32);
        lho = u32(c + 42);
        if (fn == nlen && p + 46 + fn <= z->size &&
            (nocase ? _strnicmp((const char *)c + 46, name, nlen) == 0
                    : memcmp(c + 46, name, nlen) == 0)) {
            const unsigned char *l = z->data + lho;
            DWORD start;
            unsigned char *out;
            if (lho + 30 > z->size || u32(l) != 0x04034b50) return NULL;
            start = lho + 30 + u16(l + 26) + u16(l + 28);
            if (start + csize > z->size || usize > 64u * 1024 * 1024) return NULL;
            out = xmalloc(usize + 1);
            if (method == 0) {
                memcpy(out, z->data + start, usize);
            } else if (method == 8) {
                z_stream s;
                memset(&s, 0, sizeof(s));
                if (inflateInit2(&s, -MAX_WBITS) != Z_OK) { free(out); return NULL; }
                s.next_in = z->data + start; s.avail_in = csize;
                s.next_out = out; s.avail_out = usize;
                inflate(&s, Z_FINISH);
                usize = (DWORD)s.total_out;
                inflateEnd(&s);
            } else {
                free(out);
                return NULL;
            }
            out[usize] = 0;
            if (outlen) *outlen = usize;
            return out;
        }
        p += 46 + fn + ex + cm;
    }
    return NULL;
}

static int zip_has(Zip *z, const char *name) {
    DWORD p = z->cd_offset;
    size_t nlen = strlen(name);
    unsigned i;
    for (i = 0; i < z->entries && p + 46 <= z->size; i++) {
        const unsigned char *c = z->data + p;
        unsigned fn = u16(c + 28), ex = u16(c + 30), cm = u16(c + 32);
        if (u32(c) != 0x02014b50) break;
        if (fn == nlen && memcmp(c + 46, name, nlen) == 0) return 1;
        p += 46 + fn + ex + cm;
    }
    return 0;
}

/* manifest/jad/jam text: CR and UTF-8 BOM removed, continuation lines joined */
static char *normalize_descriptor(const unsigned char *in) {
    size_t n = strlen((const char *)in), i, o = 0;
    char *out = xmalloc(n + 2);
    if (n >= 3 && in[0] == 0xef && in[1] == 0xbb && in[2] == 0xbf) in += 3;
    for (i = 0; in[i]; i++) {
        if (in[i] == '\r') continue;
        if (in[i] == '\n' && in[i + 1] == ' ') { i++; continue; }
        out[o++] = (char)in[i];
    }
    out[o++] = '\n';
    out[o] = 0;
    return out;
}

static int desc_value(const char *text, const char *key, char sep, char *out, size_t size) {
    const char *p = text;
    size_t klen = strlen(key);
    while (p && *p) {
        const char *e = strchr(p, '\n');
        const char *q;
        if (e == NULL) e = p + strlen(p);
        if ((size_t)(e - p) > klen && strncmp(p, key, klen) == 0) {
            q = p + klen;
            while (q < e && (*q == ' ' || *q == '\t')) q++;
            if (q < e && *q == sep) {
                const char *v = q + 1, *ve = e;
                size_t n;
                while (v < ve && (*v == ' ' || *v == '\t')) v++;
                while (ve > v && (ve[-1] == ' ' || ve[-1] == '\t' || ve[-1] == '\r')) ve--;
                n = (size_t)(ve - v);
                if (n >= size) n = size - 1;
                memcpy(out, v, n);
                out[n] = 0;
                return 1;
            }
        }
        p = *e ? e + 1 : e;
    }
    out[0] = 0;
    return 0;
}

static int midlet_class(const char *desc, char *out, size_t size) {
    char v[1024], *c;
    if (!desc_value(desc, "MIDlet-1", ':', v, sizeof(v))) return 0;
    c = strchr(v, ',');
    if (c) c = strchr(c + 1, ',');
    if (c == NULL) return 0;
    c++;
    while (*c == ' ') c++;
    strncpy(out, c, size - 1);
    out[size - 1] = 0;
    for (c = out + strlen(out); c > out && (c[-1] == ' ' || c[-1] == '\t'); ) *--c = 0;
    return out[0] != 0;
}

/* ======================================================== paths / games */

static wchar_t g_home[WBUF];      /* folder of the exe */
static wchar_t g_data[WBUF];      /* data folder */
static wchar_t g_ini[WBUF];       /* global settings.ini */
static char   *g_home_a;          /* ASCII form of g_home */

/* data\run, data\rms, data\games use a file-system-safe, ASCII name */
static void game_id(const wchar_t *jar, char *out, size_t size) {
    wchar_t base[WBUF], *dot;
    const wchar_t *b;
    size_t i;
    wcscpy(base, jar);
    dot = wcsrchr(base, L'.');
    if (dot && dot > wcsrchr(base, L'\\')) *dot = 0;
    b = basename_w(base);
    if (is_ascii_w(b)) {
        for (i = 0; b[i] && i < size - 1; i++) {
            wchar_t c = b[i];
            out[i] = (c == L':' || c == L'*' || c == L'?' || c == L'"' || c == L'<' ||
                      c == L'>' || c == L'|') ? '_' : (char)c;
        }
        out[i] = 0;
    } else {
        DWORD h = 5381;
        for (i = 0; jar[i]; i++) h = h * 33 + jar[i];
        for (i = 0; b[i] && i < 40 && i < size - 20; i++) {
            wchar_t c = b[i];
            out[i] = (c < 128 && (iswalnum(c) || c == L'.' || c == L'-')) ? (char)c : '_';
        }
        out[i] = 0;
        snprintf(out + strlen(out), 20, "_%lu", (unsigned long)h);
    }
}

static void game_ini_path(const wchar_t *jar, wchar_t *out) {
    char id[260];
    wchar_t *w;
    game_id(jar, id, sizeof(id));
    w = widen_utf8(id);
    _snwprintf(out, WBUF, L"%ls\\games\\%ls.ini", g_data, w);
    free(w);
}

static void game_log_path(const wchar_t *jar, wchar_t *out) {
    char id[260];
    wchar_t *w;
    game_id(jar, id, sizeof(id));
    w = widen_utf8(id);
    _snwprintf(out, WBUF, L"%ls\\logs\\%ls.log", g_data, w);
    free(w);
}

/* .jam belonging to a jar: game.jam, or the only .jam of the folder whose
 * AppClass is in the jar; jam[0] = 0 when it is a MIDP game */
static void find_jam(const wchar_t *jar, Zip *zip, wchar_t *jam) {
    wchar_t base[WBUF], dir[WBUF], tmp[WBUF], *dot;
    WIN32_FIND_DATAW fd;
    HANDLE h;
    int count = 0;
    jam[0] = 0;
    wcscpy(base, jar);
    dot = wcsrchr(base, L'.');
    if (dot && dot > wcsrchr(base, L'\\')) *dot = 0;
    _snwprintf(tmp, WBUF, L"%ls.jam", base);
    if (file_exists(tmp)) { wcscpy(jam, tmp); return; }
    _snwprintf(tmp, WBUF, L"%ls.JAM", base);
    if (file_exists(tmp)) { wcscpy(jam, tmp); return; }
    dirname_w(jar, dir);
    _snwprintf(tmp, WBUF, L"%ls\\*.jam", dir);
    h = FindFirstFileW(tmp, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            _snwprintf(tmp, WBUF, L"%ls\\%ls", dir, fd.cFileName);
            count++;
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    if (count == 1) {
        unsigned char *d;
        DWORD n;
        if (read_file(tmp, &d, &n)) {
            char *t = normalize_descriptor(d), cls[512], path[600], *c;
            if (desc_value(t, "AppClass", '=', cls, sizeof(cls)) && cls[0]) {
                for (c = cls; *c; c++) if (*c == '.') *c = '/';
                snprintf(path, sizeof(path), "%s.class", cls);
                if (zip_has(zip, path)) wcscpy(jam, tmp);
            }
            free(t);
            free(d);
        }
    }
}

/* a .jam given instead of the jar: same name, PackageURL, or the only jar */
static int jar_for_jam(const wchar_t *jam, wchar_t *jar) {
    wchar_t dir[WBUF], pat[WBUF], base[WBUF], *dot;
    unsigned char *d;
    DWORD n;
    WIN32_FIND_DATAW fd;
    HANDLE h;
    int count = 0;
    wcscpy(base, jam);
    dot = wcsrchr(base, L'.');
    if (dot) *dot = 0;
    _snwprintf(jar, WBUF, L"%ls.jar", base);
    if (file_exists(jar)) return 1;
    _snwprintf(jar, WBUF, L"%ls.JAR", base);
    if (file_exists(jar)) return 1;
    dirname_w(jam, dir);
    if (read_file(jam, &d, &n)) {
        char *t = normalize_descriptor(d), url[512], *s;
        if (desc_value(t, "PackageURL", '=', url, sizeof(url)) && url[0]) {
            s = strrchr(url, '/');
            s = s ? s + 1 : url;
            if (is_ascii_a(s)) {
                wchar_t *w = widen_utf8(s);
                _snwprintf(jar, WBUF, L"%ls\\%ls", dir, w);
                free(w);
                if (file_exists(jar)) { free(t); free(d); return 1; }
            }
        }
        free(t);
        free(d);
    }
    _snwprintf(pat, WBUF, L"%ls\\*.jar", dir);
    h = FindFirstFileW(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            _snwprintf(jar, WBUF, L"%ls\\%ls", dir, fd.cFileName);
            count++;
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    return count == 1;
}

/* ============================================================ key names */

/* SDL 1.2 key symbols (what the runtime sees) */
enum {
    K_BACKSPACE = 8, K_TAB = 9, K_RETURN = 13, K_ESCAPE = 27, K_SPACE = 32,
    K_DELETE = 127, K_KP0 = 256, K_KP_PERIOD = 266, K_KP_DIVIDE = 267,
    K_KP_MULTIPLY = 268, K_KP_MINUS = 269, K_KP_PLUS = 270, K_KP_ENTER = 271,
    K_UP = 273, K_DOWN = 274, K_RIGHT = 275, K_LEFT = 276, K_INSERT = 277,
    K_HOME = 278, K_END = 279, K_PAGEUP = 280, K_PAGEDOWN = 281, K_F1 = 282,
    K_NUMLOCK = 300, K_CAPSLOCK = 301, K_SCROLLOCK = 302, K_RSHIFT = 303,
    K_LSHIFT = 304, K_RCTRL = 305, K_LCTRL = 306, K_RALT = 307, K_LALT = 308,
    K_LSUPER = 311, K_RSUPER = 312, K_MENU = 319, K_LESS = 60, K_LAST = 323
};

/* PC scan code (set 1) -> SDL symbol, as SDL's Windows driver maps keys
 * (by position, US layout); [0x80 + code] = extended (E0) keys */
static int sdl_sym_of_scancode(unsigned sc, int extended) {
    static const unsigned short base[0x59] = {
        0, K_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', K_BACKSPACE, K_TAB,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', K_RETURN, K_LCTRL, 'a', 's',
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', K_LSHIFT, '\\', 'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',', '.', '/', K_RSHIFT, K_KP_MULTIPLY, K_LALT, K_SPACE, K_CAPSLOCK,
        K_F1, K_F1 + 1, K_F1 + 2, K_F1 + 3, K_F1 + 4,
        K_F1 + 5, K_F1 + 6, K_F1 + 7, K_F1 + 8, K_F1 + 9, K_NUMLOCK, K_SCROLLOCK,
        K_KP0 + 7, K_KP0 + 8, K_KP0 + 9, K_KP_MINUS,
        K_KP0 + 4, K_KP0 + 5, K_KP0 + 6, K_KP_PLUS, K_KP0 + 1, K_KP0 + 2, K_KP0 + 3, K_KP0,
        K_KP_PERIOD, 0, 0, K_LESS, K_F1 + 10, K_F1 + 11
    };
    if (extended) {
        switch (sc) {
        case 0x1C: return K_KP_ENTER;
        case 0x1D: return K_RCTRL;
        case 0x35: return K_KP_DIVIDE;
        case 0x38: return K_RALT;
        case 0x47: return K_HOME;
        case 0x48: return K_UP;
        case 0x49: return K_PAGEUP;
        case 0x4B: return K_LEFT;
        case 0x4D: return K_RIGHT;
        case 0x4F: return K_END;
        case 0x50: return K_DOWN;
        case 0x51: return K_PAGEDOWN;
        case 0x52: return K_INSERT;
        case 0x53: return K_DELETE;
        case 0x5B: return K_LSUPER;
        case 0x5C: return K_RSUPER;
        case 0x5D: return K_MENU;
        default: return 0;
        }
    }
    return sc < sizeof(base) / sizeof(base[0]) ? base[sc] : 0;
}

static void sym_name(int sym, wchar_t *out, size_t n) {
    const wchar_t *s = NULL;
    switch (sym) {
    case K_BACKSPACE: s = L"Backspace"; break;
    case K_TAB: s = L"Tab"; break;
    case K_RETURN: s = L"Enter"; break;
    case K_ESCAPE: s = L"Esc"; break;
    case K_SPACE: s = L"Space"; break;
    case K_DELETE: s = L"Delete"; break;
    case K_KP_PERIOD: s = L"Num ."; break;
    case K_KP_DIVIDE: s = L"Num /"; break;
    case K_KP_MULTIPLY: s = L"Num *"; break;
    case K_KP_MINUS: s = L"Num -"; break;
    case K_KP_PLUS: s = L"Num +"; break;
    case K_KP_ENTER: s = L"Num Enter"; break;
    case K_UP: s = L"Up"; break;
    case K_DOWN: s = L"Down"; break;
    case K_RIGHT: s = L"Right"; break;
    case K_LEFT: s = L"Left"; break;
    case K_INSERT: s = L"Insert"; break;
    case K_HOME: s = L"Home"; break;
    case K_END: s = L"End"; break;
    case K_PAGEUP: s = L"Page Up"; break;
    case K_PAGEDOWN: s = L"Page Down"; break;
    case K_NUMLOCK: s = L"Num Lock"; break;
    case K_CAPSLOCK: s = L"Caps Lock"; break;
    case K_SCROLLOCK: s = L"Scroll Lock"; break;
    case K_RSHIFT: s = L"Right Shift"; break;
    case K_LSHIFT: s = L"Left Shift"; break;
    case K_RCTRL: s = L"Right Ctrl"; break;
    case K_LCTRL: s = L"Left Ctrl"; break;
    case K_RALT: s = L"Right Alt"; break;
    case K_LALT: s = L"Left Alt"; break;
    case K_LSUPER: s = L"Left Win"; break;
    case K_RSUPER: s = L"Right Win"; break;
    case K_MENU: s = L"Menu"; break;
    case K_LESS: s = L"<"; break;
    }
    if (s) { wcsncpy(out, s, n - 1); out[n - 1] = 0; return; }
    if (sym >= K_KP0 && sym <= K_KP0 + 9) { _snwprintf(out, n, L"Num %d", sym - K_KP0); return; }
    if (sym >= K_F1 && sym <= K_F1 + 14) { _snwprintf(out, n, L"F%d", sym - K_F1 + 1); return; }
    if (sym > 32 && sym < 127) {
        _snwprintf(out, n, L"%lc", (wchar_t)(sym >= 'a' && sym <= 'z' ? sym - 32 : sym));
        return;
    }
    _snwprintf(out, n, L"#%d", sym);
}

/* ======================================================== phone actions */

#define ACT_SOFT1 -1001   /* resolved through the soft key type */
#define ACT_SOFT2 -1002
#define ACT_MENU  1000    /* runtime: open the runtime menu */
#define ACT_FULL  1001    /* runtime: full screen */
#define MAX_BIND  3

typedef struct {
    const char *id;
    const wchar_t *label;
    int code;
    int defaults[MAX_BIND];
} Action;

static const Action Actions[] = {
    { "up",     L"Up",                      -1,  { K_UP, 'w', 0 } },
    { "down",   L"Down",                    -2,  { K_DOWN, 's', 0 } },
    { "left",   L"Left",                    -3,  { K_LEFT, 'a', 0 } },
    { "right",  L"Right",                   -4,  { K_RIGHT, 'd', 0 } },
    { "fire",   L"Fire / OK",               -5,  { K_RETURN, K_SPACE, 0 } },
    { "soft1",  L"Left soft key",           ACT_SOFT1, { K_F1, K_TAB, 'q' } },
    { "soft2",  L"Right soft key",          ACT_SOFT2, { K_F1 + 1, K_BACKSPACE, 'e' } },
    { "1",      L"1",                       '1', { '1', K_KP0 + 7, 0 } },
    { "2",      L"2",                       '2', { '2', K_KP0 + 8, 0 } },
    { "3",      L"3",                       '3', { '3', K_KP0 + 9, 0 } },
    { "4",      L"4",                       '4', { '4', K_KP0 + 4, 0 } },
    { "5",      L"5",                       '5', { '5', K_KP0 + 5, 0 } },
    { "6",      L"6",                       '6', { '6', K_KP0 + 6, 0 } },
    { "7",      L"7",                       '7', { '7', K_KP0 + 1, 0 } },
    { "8",      L"8",                       '8', { '8', K_KP0 + 2, 0 } },
    { "9",      L"9",                       '9', { '9', K_KP0 + 3, 0 } },
    { "0",      L"0",                       '0', { '0', K_KP0, 0 } },
    { "star",   L"*",                       '*', { K_KP_MULTIPLY, 'z', 0 } },
    { "pound",  L"#",                       '#', { K_KP_PLUS, 'x', 0 } },
    { "clear",  L"Clear (C)",               -8,  { K_DELETE, 0, 0 } },
    { "send",   L"Call (green)",            -10, { K_HOME, 0, 0 } },
    { "end",    L"Hang up (red)",           -11, { K_END, 0, 0 } },
    { "gamea",  L"Game A",                  -13, { K_F1 + 8, 0, 0 } },
    { "gameb",  L"Game B",                  -14, { K_F1 + 9, 0, 0 } },
    { "gamec",  L"Game C",                  -15, { K_PAGEUP, 0, 0 } },
    { "gamed",  L"Game D",                  -16, { K_PAGEDOWN, K_F1 + 11, 0 } },
    { "menu",   L"[Runtime menu]",          ACT_MENU, { K_ESCAPE, 0, 0 } },
    { "full",   L"[Full screen]",           ACT_FULL, { K_F1 + 10, 0, 0 } }
};
#define ACTION_COUNT ((int)(sizeof(Actions) / sizeof(Actions[0])))

static const struct { const char *id; const wchar_t *label; int soft1, soft2; } Phones[] = {
    { "standard",  L"Nokia / SE / Samsung (-6/-7)", -6, -7 },
    { "motorola",  L"Motorola (-21/-22)", -21, -22 },
    { "moto-old",  L"Motorola old (21/22)", 21, 22 },
    { "siemens",   L"Siemens (-1/-4)", -1, -4 }
};
#define PHONE_COUNT ((int)(sizeof(Phones) / sizeof(Phones[0])))

/* ============================================================= settings */

typedef struct {
    int scale;            /* 1..8 */
    int fullscreen;
    char mode[16];        /* auto fit integer crop fill center */
    char filter[16];      /* sharp smooth pixel scale2x */
    char lcd[32];         /* "" = auto, or WxH */
    char fps[8];          /* off 20 25 30 50 60 */
    int heap;             /* MB, 0 = default */
    int jit, verify;
    char vmargs[256];
    int mute, volume;
    char synth[16];
    int log, console, log_keys, log_events, log_display, show_fps;
    char phone[16];
    int keys[ACTION_COUNT][MAX_BIND];
    wchar_t games_dir[WBUF];   /* global only */
} Settings;

static void settings_default(Settings *s) {
    int i, j;
    memset(s, 0, sizeof(*s));
    s->scale = 3;
    strcpy(s->mode, "fit");
    strcpy(s->filter, "sharp");
    strcpy(s->fps, "30");
    s->jit = 1;
    s->volume = 100;
    strcpy(s->synth, "hybrid");
    s->log = 1;
    strcpy(s->phone, "standard");
    for (i = 0; i < ACTION_COUNT; i++)
        for (j = 0; j < MAX_BIND; j++) s->keys[i][j] = Actions[i].defaults[j];
    _snwprintf(s->games_dir, WBUF, L"%ls\\games", g_home);
}

static void copy_str(char *dst, size_t n, const char *v) {
    strncpy(dst, v, n - 1);
    dst[n - 1] = 0;
}

static void keys_parse(Settings *s, const char *v) {
    /* "up=273,119;down=274;..." : listed actions are replaced */
    while (*v) {
        char id[32];
        size_t n = 0;
        int i, a = -1, k = 0;
        while (*v == ' ' || *v == ';') v++;
        while (*v && *v != '=' && *v != ';' && n < sizeof(id) - 1) id[n++] = *v++;
        id[n] = 0;
        if (*v != '=') { while (*v && *v != ';') v++; continue; }
        v++;
        for (i = 0; i < ACTION_COUNT; i++) if (strcmp(Actions[i].id, id) == 0) a = i;
        if (a >= 0) for (i = 0; i < MAX_BIND; i++) s->keys[a][i] = 0;
        while (*v && *v != ';') {
            char *end;
            long sym = strtol(v, &end, 10);
            if (end == v) { v++; continue; }
            v = end;
            if (*v == ',') v++;
            if (a >= 0 && sym > 0 && sym < K_LAST && k < MAX_BIND) s->keys[a][k++] = (int)sym;
        }
    }
}

static void settings_apply(Settings *s, const char *key, const char *v) {
    if (!_stricmp(key, "SCALE")) { s->scale = atoi(v); if (s->scale < 1) s->scale = 1; if (s->scale > 8) s->scale = 8; }
    else if (!_stricmp(key, "FULLSCREEN")) s->fullscreen = atoi(v) != 0;
    else if (!_stricmp(key, "MODE")) copy_str(s->mode, sizeof(s->mode), v);
    else if (!_stricmp(key, "FILTER")) copy_str(s->filter, sizeof(s->filter), v);
    else if (!_stricmp(key, "LCD")) copy_str(s->lcd, sizeof(s->lcd), v);
    else if (!_stricmp(key, "FPS")) copy_str(s->fps, sizeof(s->fps), v);
    else if (!_stricmp(key, "HEAP_MB")) s->heap = atoi(v);
    else if (!_stricmp(key, "JIT")) s->jit = atoi(v) != 0;
    else if (!_stricmp(key, "VERIFY")) s->verify = atoi(v) != 0;
    else if (!_stricmp(key, "VMARGS")) copy_str(s->vmargs, sizeof(s->vmargs), v);
    else if (!_stricmp(key, "MUTE")) s->mute = atoi(v) != 0;
    else if (!_stricmp(key, "VOLUME")) { s->volume = atoi(v); if (s->volume < 0) s->volume = 0; if (s->volume > 100) s->volume = 100; }
    else if (!_stricmp(key, "SYNTH")) copy_str(s->synth, sizeof(s->synth), v);
    else if (!_stricmp(key, "LOG")) s->log = atoi(v) != 0;
    else if (!_stricmp(key, "CONSOLE")) s->console = atoi(v) != 0;
    else if (!_stricmp(key, "LOG_KEYS")) s->log_keys = atoi(v) != 0;
    else if (!_stricmp(key, "LOG_EVENTS")) s->log_events = atoi(v) != 0;
    else if (!_stricmp(key, "LOG_DISPLAY")) s->log_display = atoi(v) != 0;
    else if (!_stricmp(key, "SHOW_FPS")) s->show_fps = atoi(v) != 0;
    else if (!_stricmp(key, "PHONE")) copy_str(s->phone, sizeof(s->phone), v);
    else if (!_stricmp(key, "KEYS")) keys_parse(s, v);
    else if (!_stricmp(key, "GAMES_DIR") && v[0]) {
        wchar_t *w = widen_utf8(v);
        wcsncpy(s->games_dir, w, WBUF - 1);
        free(w);
    }
}

static int settings_load_file(Settings *s, const wchar_t *path) {
    unsigned char *d;
    DWORD n;
    char *p;
    if (!read_file(path, &d, &n)) return 0;
    p = (char *)d;
    if (n >= 3 && (unsigned char)p[0] == 0xef && (unsigned char)p[1] == 0xbb) p += 3;
    while (*p) {
        char *e = strchr(p, '\n'), *eq, *k, *v, *t;
        if (e) *e = 0;
        k = p;
        while (*k == ' ' || *k == '\t') k++;
        eq = strchr(k, '=');
        if (*k != '#' && *k != ';' && *k != '[' && eq) {
            *eq = 0;
            for (t = eq; t > k && (t[-1] == ' ' || t[-1] == '\t'); ) *--t = 0;
            v = eq + 1;
            while (*v == ' ' || *v == '\t') v++;
            t = v + strlen(v);
            while (t > v && (t[-1] == '\r' || t[-1] == ' ' || t[-1] == '\t')) *--t = 0;
            if (*v == '"' && t > v + 1 && t[-1] == '"') { v++; t[-1] = 0; }
            settings_apply(s, k, v);
        }
        if (!e) break;
        p = e + 1;
    }
    free(d);
    return 1;
}

/* environment variables of the same name win (old settings.ini keys) */
static void settings_env(Settings *s) {
    static const char *keys[] = { "SCALE", "FULLSCREEN", "MODE", "FILTER", "LCD", "FPS",
        "HEAP_MB", "JIT", "VERIFY", "VMARGS", "MUTE", "VOLUME", "SYNTH", "CONSOLE", "PHONE" };
    char v[512];
    size_t i;
    for (i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        DWORD n = GetEnvironmentVariableA(keys[i], v, sizeof(v));
        if (n > 0 && n < sizeof(v)) settings_apply(s, keys[i], v);
    }
}

/* effective settings of a game (jar may be NULL): defaults, settings.ini,
 * data\games\<game>.ini */
static void settings_for(Settings *s, const wchar_t *jar) {
    settings_default(s);
    settings_load_file(s, g_ini);
    if (jar != NULL && jar[0]) {
        wchar_t gi[WBUF];
        game_ini_path(jar, gi);
        settings_load_file(s, gi);
    }
}

static void keys_format(const Settings *s, char *out, size_t size) {
    size_t o = 0;
    int i, j;
    out[0] = 0;
    for (i = 0; i < ACTION_COUNT && o + 64 < size; i++) {
        o += (size_t)snprintf(out + o, size - o, "%s%s=", i ? ";" : "", Actions[i].id);
        for (j = 0; j < MAX_BIND; j++) {
            if (s->keys[i][j]) o += (size_t)snprintf(out + o, size - o, "%s%d",
                                                     out[o - 1] == '=' ? "" : ",", s->keys[i][j]);
        }
    }
}

static int settings_save_file(const Settings *s, const wchar_t *path, int global) {
    char buf[8192], keys[4096];
    int n;
    keys_format(s, keys, sizeof(keys));
    n = snprintf(buf, sizeof(buf),
        "# FunKey J2ME settings (written by FunKeyJ2ME.exe; edit with the launcher)\r\n"
        "SCALE = %d\r\nFULLSCREEN = %d\r\nMODE = %s\r\nFILTER = %s\r\nLCD = %s\r\n"
        "FPS = %s\r\nHEAP_MB = %d\r\nJIT = %d\r\nVERIFY = %d\r\nVMARGS = %s\r\n"
        "MUTE = %d\r\nVOLUME = %d\r\nSYNTH = %s\r\n"
        "LOG = %d\r\nCONSOLE = %d\r\nLOG_KEYS = %d\r\nLOG_EVENTS = %d\r\nLOG_DISPLAY = %d\r\n"
        "SHOW_FPS = %d\r\nPHONE = %s\r\nKEYS = %s\r\n",
        s->scale, s->fullscreen, s->mode, s->filter, s->lcd, s->fps, s->heap, s->jit,
        s->verify, s->vmargs, s->mute, s->volume, s->synth, s->log, s->console,
        s->log_keys, s->log_events, s->log_display, s->show_fps, s->phone, keys);
    if (global && n > 0 && n < (int)sizeof(buf)) {
        char *gd = utf8_of(s->games_dir);
        n += snprintf(buf + n, sizeof(buf) - (size_t)n, "GAMES_DIR = %s\r\n", gd);
        free(gd);
    }
    if (n <= 0 || n >= (int)sizeof(buf)) return 0;
    {
        wchar_t dir[WBUF];
        dirname_w(path, dir);
        mkdirs(dir);
    }
    return write_file(path, buf, (DWORD)n);
}

static int phone_index(const char *id) {
    int i;
    for (i = 0; i < PHONE_COUNT; i++) if (strcmp(Phones[i].id, id) == 0) return i;
    return 0;
}

/* "sym=code;..." for PHONEME_PC_KEYS */
static void pc_keys_env(const Settings *s, char *out, size_t size) {
    size_t o = 0;
    int i, j, p = phone_index(s->phone);
    out[0] = 0;
    for (i = 0; i < ACTION_COUNT; i++) {
        int code = Actions[i].code;
        if (code == ACT_SOFT1) code = Phones[p].soft1;
        if (code == ACT_SOFT2) code = Phones[p].soft2;
        for (j = 0; j < MAX_BIND; j++) {
            if (s->keys[i][j] && o + 24 < size)
                o += (size_t)snprintf(out + o, size - o, "%s%d=%d", o ? ";" : "", s->keys[i][j], code);
        }
    }
}

/* ================================================================= run */

static wchar_t *cmd;
static size_t cmd_len, cmd_cap;

static void cmd_raw(const wchar_t *s) {
    size_t n = wcslen(s);
    if (cmd_len + n + 1 > cmd_cap) {
        cmd_cap = (cmd_len + n + 1) * 2 + 1024;
        cmd = realloc(cmd, cmd_cap * sizeof(wchar_t));
        if (cmd == NULL) ExitProcess(1);
    }
    memcpy(cmd + cmd_len, s, (n + 1) * sizeof(wchar_t));
    cmd_len += n;
}

static void cmd_arg_w(const wchar_t *a) {
    const wchar_t *p;
    int quote = *a == 0 || wcspbrk(a, L" \t\"") != NULL;
    if (cmd_len) cmd_raw(L" ");
    if (!quote) { cmd_raw(a); return; }
    cmd_raw(L"\"");
    for (p = a; ; p++) {
        unsigned bs = 0;
        while (*p == L'\\') { bs++; p++; }
        if (*p == 0) {
            while (bs--) cmd_raw(L"\\\\");
            break;
        }
        if (*p == L'"') {
            while (bs--) cmd_raw(L"\\\\");
            cmd_raw(L"\\\"");
        } else {
            wchar_t one[2] = { *p, 0 };
            while (bs--) cmd_raw(L"\\");
            cmd_raw(one);
        }
    }
    cmd_raw(L"\"");
}

static void cmd_arg(const char *a) {
    wchar_t *w = widen_utf8(a);
    cmd_arg_w(w);
    free(w);
}

static void cmd_argf(const char *fmt, const char *v) {
    size_t n = strlen(fmt) + strlen(v) + 1;
    char *s = xmalloc(n);
    snprintf(s, n, fmt, v);
    cmd_arg(s);
    free(s);
}

static void cmd_args_split(const char *s) {
    char buf[1024];
    while (*s) {
        size_t n = 0;
        while (*s == ' ' || *s == '\t') s++;
        while (*s && *s != ' ' && *s != '\t' && n < sizeof(buf) - 1) buf[n++] = *s++;
        buf[n] = 0;
        if (n) cmd_arg(buf);
    }
}

/* environment block for the game: ours plus the PHONEME_ variables */
typedef struct { char *k, *v; } EnvVar;
static EnvVar env_vars[64];
static int env_count;

static void env_set(const char *k, const char *v) {
    int i;
    for (i = 0; i < env_count; i++) {
        if (strcmp(env_vars[i].k, k) == 0) {
            free(env_vars[i].v);
            env_vars[i].v = _strdup(v);
            return;
        }
    }
    if (env_count < 64) {
        env_vars[env_count].k = _strdup(k);
        env_vars[env_count].v = _strdup(v);
        env_count++;
    }
}

static void env_reset(void) {
    int i;
    for (i = 0; i < env_count; i++) { free(env_vars[i].k); free(env_vars[i].v); }
    env_count = 0;
}

static wchar_t *env_block(void) {
    wchar_t *cur = GetEnvironmentStringsW(), *p, *out;
    size_t n = 0, o = 0;
    int i;
    for (p = cur; *p; p += wcslen(p) + 1) n += wcslen(p) + 1;
    for (i = 0; i < env_count; i++) n += strlen(env_vars[i].k) + strlen(env_vars[i].v) * 2 + 4;
    out = xmalloc((n + 2) * sizeof(wchar_t));
    for (p = cur; *p; p += wcslen(p) + 1) {
        int skip = 0;
        for (i = 0; i < env_count; i++) {
            size_t kl = strlen(env_vars[i].k);
            if (wcslen(p) > kl && p[kl] == L'=') {
                wchar_t kw[128];
                size_t j;
                for (j = 0; j < kl && j < 127; j++) kw[j] = (wchar_t)env_vars[i].k[j];
                kw[j] = 0;
                if (_wcsnicmp(p, kw, kl) == 0) skip = 1;
            }
        }
        if (!skip) { wcscpy(out + o, p); o += wcslen(p) + 1; }
    }
    for (i = 0; i < env_count; i++) {
        wchar_t *k = widen_utf8(env_vars[i].k), *v = widen_utf8(env_vars[i].v);
        swprintf(out + o, n + 2 - o, L"%ls=%ls", k, v);
        o += wcslen(out + o) + 1;
        free(k);
        free(v);
    }
    out[o] = 0;
    FreeEnvironmentStringsW(cur);
    return out;
}

typedef struct {
    HANDLE process;
    HWND notify;
    wchar_t log[WBUF];
    wchar_t sp_orig[WBUF], sp_copy[WBUF];
    int show_errors;
    DWORD code;
} RunInfo;

static void show_log_tail(HWND owner, const wchar_t *log, DWORD code) {
    unsigned char *d;
    DWORD n;
    wchar_t msg[6000];
    if (log[0] && read_file(log, &d, &n)) {
        DWORD start = n > 1800 ? n - 1800 : 0;
        wchar_t *w;
        while (start > 0 && start < n && d[start - 1] != '\n') start++;
        w = widen_utf8((char *)d + start);
        _snwprintf(msg, 5999, L"The game stopped with an error (code %lu).\n\n"
                   L"End of the log (%ls):\n\n%ls", (unsigned long)code, log, w);
        free(w);
        free(d);
    } else {
        _snwprintf(msg, 5999, L"The game stopped with an error (code %lu).", (unsigned long)code);
    }
    msg[5999] = 0;
    MessageBoxW(owner, msg, APP_TITLE, MB_OK | MB_ICONWARNING);
}

static DWORD WINAPI run_waiter(LPVOID p) {
    RunInfo *ri = (RunInfo *)p;
    WaitForSingleObject(ri->process, INFINITE);
    GetExitCodeProcess(ri->process, &ri->code);
    CloseHandle(ri->process);
    if (ri->sp_copy[0] && ri->sp_orig[0] && !CopyFileW(ri->sp_copy, ri->sp_orig, FALSE))
        MessageBoxW(ri->notify, L"Could not write the scratchpad (save) back to the game folder.",
                    APP_TITLE, MB_OK | MB_ICONWARNING);
    /* 1 = MIDP_SHUTDOWN_STATUS: the window was closed or the game quit */
    if (ri->code != 0 && ri->code != 1 && ri->show_errors) show_log_tail(ri->notify, ri->log, ri->code);
    if (ri->notify) PostMessageW(ri->notify, WM_APP_GAME_ENDED, 0, 0);
    free(ri);
    return 0;
}

/* starts a game; wait = 1: returns when it ends (drag & drop mode) */
static int run_game(HWND owner, const wchar_t *path, int wait) {
    wchar_t jar[WBUF], jam[WBUF], dir[WBUF], base[WBUF], tmp[WBUF];
    wchar_t rundir[WBUF], rmsdir[WBUF], logpath[WBUF], *dot, *envb;
    char id[260], s[4096], *jar_a = NULL, *r;
    Settings st;
    Zip zip;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    HANDLE logh = INVALID_HANDLE_VALUE;
    RunInfo *ri;
    char title[600] = "";

    if (!GetFullPathNameW(path, WBUF, jar, NULL)) { fail(L"Bad path: %ls", path); return 0; }
    jam[0] = 0;
    dot = wcsrchr(jar, L'.');
    if (dot && _wcsicmp(dot, L".jam") == 0) {
        wcscpy(jam, jar);
        if (!jar_for_jam(jam, jar)) { fail(L"No jar found for %ls", jam); return 0; }
    }
    if (!file_exists(jar)) { fail(L"No such file: %ls", jar); return 0; }
    if (!zip_open(&zip, jar)) { fail(L"%ls is not a jar (zip) file.", jar); return 0; }
    if (jam[0] == 0) find_jam(jar, &zip, jam);
    settings_for(&st, jar);
    settings_env(&st);

    dirname_w(jar, dir);
    wcscpy(base, jar);
    dot = wcsrchr(base, L'.');
    if (dot && dot > wcsrchr(base, L'\\')) *dot = 0;
    game_id(jar, id, sizeof(id));
    {
        wchar_t *nw = widen_utf8(id);
        _snwprintf(rundir, WBUF, L"%ls\\run\\%ls", g_data, nw);
        _snwprintf(rmsdir, WBUF, L"%ls\\rms\\%ls", g_data, nw);
        free(nw);
    }
    game_log_path(jar, logpath);
    mkdirs(rundir);
    mkdirs(rmsdir);
    _snwprintf(tmp, WBUF, L"%ls\\logs", g_data);
    mkdirs(tmp);

    env_reset();
    env_set("MIDP_HOME", g_home_a);
    r = ascii_alias(rmsdir);
    if (r == NULL) { fail(L"Bad data folder %ls", rmsdir); zip_close(&zip); return 0; }
    env_set("PHONEME_RMS_HOME", r);
    free(r);
    snprintf(s, sizeof(s), "%d", st.scale); env_set("PHONEME_WINDOW_SCALE", s);
    env_set("PHONEME_DESKTOP", "1");
    env_set("PHONEME_FULLSCREEN", st.fullscreen ? "1" : "0");
    env_set("PHONEME_SCALE_MODE", st.mode);
    env_set("PHONEME_SCALE_FILTER", st.filter);
    env_set("PHONEME_FPS_LIMIT", st.fps);
    if (!st.mute) env_set("PHONEME_ENABLE_AUDIO", "1");
    snprintf(s, sizeof(s), "%d", st.volume); env_set("PHONEME_SOUND_VOLUME", s);
    env_set("PHONEME_SYNTH", st.synth);
    if (st.log_keys) env_set("PHONEME_KEY_DEBUG", "1");
    if (st.log_events) env_set("PHONEME_EVENT_DEBUG", "1");
    if (st.log_display) env_set("PHONEME_DISPLAY_DEBUG", "1");
    if (st.show_fps) env_set("PHONEME_SHOW_FPS", "1");
    pc_keys_env(&st, s, sizeof(s));
    env_set("PHONEME_PC_KEYS", s);
    if (st.lcd[0]) { env_set("PHONEME_LCD_SIZE", st.lcd); env_set("PHONEME_SOURCE_SIZE", st.lcd); }

    /* command line */
    cmd_len = 0;
    _snwprintf(tmp, WBUF, L"%ls\\bin\\runMidlet.exe", g_home);
    cmd_arg_w(tmp);
    if (!st.verify) cmd_arg("-UseVerifier");
    if (!st.jit) cmd_arg("-UseCompiler");
    cmd_args_split(st.vmargs);

    ri = (RunInfo *)calloc(1, sizeof(RunInfo));
    if (ri == NULL) { zip_close(&zip); return 0; }

    if (jam[0]) {
        unsigned char *jd;
        DWORD jn, i;
        char *jt, area[64], param[1024], ver[256], spsize[64], *hex, *sp_a, *pics_a = NULL;
        char app_class[512], name[512];
        wchar_t pics[WBUF], sp_orig[WBUF];

        if (!read_file(jam, &jd, &jn)) { fail(L"Cannot read %ls", jam); zip_close(&zip); free(ri); return 0; }
        jt = normalize_descriptor(jd);
        desc_value(jt, "AppClass", '=', app_class, sizeof(app_class));
        if (!app_class[0]) { fail(L"No AppClass in %ls", jam); free(jt); free(jd); zip_close(&zip); free(ri); return 0; }
        if (desc_value(jt, "AppName", '=', name, sizeof(name)) && name[0]) {
            wchar_t *w = widen_cp(name, 932);    /* .jam files are Shift_JIS */
            char *u = utf8_of(w);
            copy_str(title, sizeof(title), u);
            free(u);
            free(w);
        }
        snprintf(s, sizeof(s), "%d", st.heap > 0 ? st.heap : 32); env_set("PHONEME_HEAP_MB", s);
        desc_value(jt, "DrawArea", '=', area, sizeof(area));
        if (!(area[0] >= '0' && area[0] <= '9' && strchr(area, 'x'))) strcpy(area, "240x240");
        if (!st.lcd[0]) { env_set("PHONEME_LCD_SIZE", area); env_set("PHONEME_SOURCE_SIZE", area); }
        hex = xmalloc((size_t)jn * 2 + 1);
        for (i = 0; i < jn; i++) sprintf(hex + i * 2, "%02x", jd[i]);
        hex[jn * 2] = 0;

        /* scratchpad: game.sp / game.SP; created empty */
        _snwprintf(sp_orig, WBUF, L"%ls.sp", base);
        _snwprintf(tmp, WBUF, L"%ls.SP", base);
        if (!file_exists(sp_orig) && file_exists(tmp)) wcscpy(sp_orig, tmp);
        if (!file_exists(sp_orig)) write_file(sp_orig, "", 0);
        sp_a = file_exists(sp_orig) ? ascii_alias(sp_orig) : NULL;
        if (sp_a != NULL && !is_ascii_w(basename_w(sp_orig))) { free(sp_a); sp_a = NULL; }
        if (sp_a == NULL) {
            wchar_t *w = widen_utf8(id);
            _snwprintf(ri->sp_copy, WBUF, L"%ls\\%ls.sp", rundir, w);
            free(w);
            if (file_exists(sp_orig)) CopyFileW(sp_orig, ri->sp_copy, FALSE);
            else write_file(ri->sp_copy, "", 0);
            wcscpy(ri->sp_orig, sp_orig);
            sp_a = ascii_alias(ri->sp_copy);
            if (sp_a == NULL) sp_a = _strdup("");
        }

        pics[0] = 0;
        _snwprintf(tmp, WBUF, L"%ls\\mypicture", dir);
        if (dir_exists(tmp)) wcscpy(pics, tmp);
        else {
            _snwprintf(tmp, WBUF, L"%ls.pictures", base);
            if (dir_exists(tmp)) wcscpy(pics, tmp);
        }
        if (pics[0]) pics_a = ascii_alias(pics);

        jar_a = ascii_alias(jar);
        if (jar_a == NULL) {
            _snwprintf(tmp, WBUF, L"%ls\\game.jar", rundir);
            if (!link_or_copy(jar, tmp)) {
                fail(L"Cannot copy %ls\n(is the game still running?)", jar);
                free(hex); free(sp_a); free(pics_a); free(jt); free(jd); zip_close(&zip); free(ri);
                return 0;
            }
            jar_a = ascii_alias(tmp);
        }

        desc_value(jt, "AppParam", '=', param, sizeof(param));
        desc_value(jt, "AppVer", '=', ver, sizeof(ver));
        desc_value(jt, "SPsize", '=', spsize, sizeof(spsize));
        if (!is_ascii_a(param)) param[0] = 0;   /* SJIS values travel in doja.jam.hex */
        if (!is_ascii_a(ver)) ver[0] = 0;

        cmd_arg("=MaxMethodToCompile3000");
        cmd_argf("-Ddoja.app.class=%s", app_class);
        cmd_argf("-Ddoja.app.param=%s", param);
        cmd_argf("-Ddoja.app.ver=%s", ver);
        cmd_arg("-Dmicroedition.encoding=SJIS");
        cmd_argf("-Ddoja.scratchpad.path=%s", sp_a);
        cmd_argf("-Ddoja.spsize=%s", spsize);
        cmd_argf("-Ddoja.jam.hex=%s", hex);
        cmd_argf("-Ddoja.pictures=%s", pics_a ? pics_a : "");
        cmd_arg("-classpathext");
        snprintf(s, sizeof(s), "%s;%s\\lib\\doja-support.jar", jar_a, g_home_a);
        cmd_arg(s);
        cmd_arg("internal");
        cmd_arg("com.funkey.doja.DoJaMIDlet");
        free(hex);
        free(sp_a);
        free(pics_a);
        free(jt);
        free(jd);
    } else {
        unsigned char *d = NULL, *mf;
        DWORD n;
        char *desc = NULL, main_class[512] = "", name[512];
        int own_jad = 0;

        snprintf(s, sizeof(s), "%d", st.heap > 0 ? st.heap : 16); env_set("PHONEME_HEAP_MB", s);
        _snwprintf(tmp, WBUF, L"%ls.jad", base);
        if (read_file(tmp, &d, &n)) {
            desc = normalize_descriptor(d);
            free(d);
            if (midlet_class(desc, main_class, sizeof(main_class))) own_jad = 1;
            else { free(desc); desc = NULL; }
        }
        mf = zip_read(&zip, "META-INF/MANIFEST.MF", 1, &n);
        if (!own_jad) {
            if (mf == NULL) { fail(L"%ls has no manifest.", jar); zip_close(&zip); free(ri); return 0; }
            desc = normalize_descriptor(mf);
            midlet_class(desc, main_class, sizeof(main_class));
        } else if (!main_class[0] && mf) {
            char *m = normalize_descriptor(mf);
            midlet_class(m, main_class, sizeof(main_class));
            free(m);
        }
        if (!main_class[0]) { fail(L"No MIDlet-1 entry in %ls", jar); zip_close(&zip); free(ri); return 0; }
        if (desc_value(desc, "MIDlet-Name", ':', name, sizeof(name)) && name[0])
            copy_str(title, sizeof(title), name);

        jar_a = own_jad ? ascii_alias(jar) : NULL;
        if (jar_a != NULL) {
            /* the .jad must be reachable by the same (short) base name */
            char jad_check[WBUF];
            snprintf(jad_check, sizeof(jad_check), "%s", jar_a);
            if (strlen(jad_check) > 4) strcpy(jad_check + strlen(jad_check) - 4, ".jad");
            if (GetFileAttributesA(jad_check) == INVALID_FILE_ATTRIBUTES) { free(jar_a); jar_a = NULL; }
        }
        if (jar_a == NULL) {
            _snwprintf(tmp, WBUF, L"%ls\\game.jar", rundir);
            if (!link_or_copy(jar, tmp)) {
                fail(L"Cannot copy %ls\n(is the game still running?)", jar);
                free(desc); if (mf) free(mf); zip_close(&zip); free(ri); return 0;
            }
            jar_a = ascii_alias(tmp);
            _snwprintf(tmp, WBUF, L"%ls\\game.jad", rundir);
            if (own_jad) {
                wchar_t src[WBUF];
                _snwprintf(src, WBUF, L"%ls.jad", base);
                CopyFileW(src, tmp, FALSE);
            } else {
                write_file(tmp, desc, (DWORD)strlen(desc));
            }
        }
        if (mf) free(mf);
        free(desc);
        cmd_arg("-classpathext");
        cmd_arg(jar_a);
        cmd_arg("internal");
        cmd_arg(main_class);
    }
    zip_close(&zip);
    free(jar_a);
    if (!title[0]) {
        char *u = utf8_of(basename_w(base));
        copy_str(title, sizeof(title), u);
        free(u);
    }
    env_set("PHONEME_WINDOW_TITLE", title);

    /* start: log file / console / nothing */
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    if (!st.console) {
        logh = CreateFileW(st.log ? logpath : L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           &sa, st.log ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (logh != INVALID_HANDLE_VALUE) {
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdInput = NULL;
            si.hStdOutput = logh;
            si.hStdError = logh;
        }
    }
    envb = env_block();
    {
        wchar_t bindir[WBUF];
        _snwprintf(bindir, WBUF, L"%ls\\bin", g_home);
        if (!CreateProcessW(NULL, cmd, NULL, NULL, TRUE,
                            (st.console ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW) | CREATE_UNICODE_ENVIRONMENT,
                            envb, bindir, &si, &pi)) {
            fail(L"Cannot start bin\\runMidlet.exe (error %lu).", (unsigned long)GetLastError());
            free(envb);
            if (logh != INVALID_HANDLE_VALUE) CloseHandle(logh);
            free(ri);
            return 0;
        }
    }
    free(envb);
    if (logh != INVALID_HANDLE_VALUE) CloseHandle(logh);
    CloseHandle(pi.hThread);

    ri->process = pi.hProcess;
    ri->notify = owner;
    ri->show_errors = !st.console;
    if (st.log && !st.console) wcscpy(ri->log, logpath);
    if (wait) {
        run_waiter(ri);
    } else {
        HANDLE th = CreateThread(NULL, 0, run_waiter, ri, 0, NULL);
        if (th) CloseHandle(th);
    }
    return 1;
}

/* =================================================================== GUI */

static HWND g_main, g_tab, g_pages[4];
static HWND g_games, g_keys;
static HINSTANCE g_inst;
static Settings g_set;            /* what the pages show */
static int g_scope_game;          /* editing the selected game's own settings */
static int g_loading;             /* filling controls: ignore notifications */
static wchar_t g_sel_jar[WBUF];   /* selected game */

typedef struct { wchar_t path[WBUF]; } GameItem;

static const struct { const char *id; const wchar_t *label; } Modes[] = {
    { "fit", L"Fit (keep aspect)" }, { "integer", L"Integer scale" },
    { "auto", L"Auto" }, { "crop", L"Fill, crop edges" },
    { "fill", L"Stretch" }, { "center", L"Center, no scaling" }
};
static const struct { const char *id; const wchar_t *label; } Filters[] = {
    { "sharp", L"Sharp (clean pixels)" }, { "scale2x", L"Scale2x (smooth pixel art)" },
    { "smooth", L"Smooth (bilinear)" }, { "pixel", L"Pixel (nearest)" }
};
static const struct { const char *id; const wchar_t *label; } Fpss[] = {
    { "30", L"30 FPS" }, { "60", L"60 FPS" }, { "50", L"50 FPS" }, { "25", L"25 FPS" },
    { "20", L"20 FPS" }, { "off", L"No limit" }
};
static const struct { const char *id; const wchar_t *label; } Synths[] = {
    { "hybrid", L"Hybrid" }, { "wavetable", L"Wavetable" }, { "fm", L"FM" }
};
static const wchar_t *Lcds[] = {
    L"Auto", L"128x128", L"128x160", L"176x208", L"176x220", L"208x208", L"240x240",
    L"240x320", L"320x240", L"240x400", L"352x416", L"360x640", L"480x800", NULL
};
static const int Heaps[] = { 0, 8, 16, 24, 32, 48, 64 };

#define N(a) ((int)(sizeof(a) / sizeof((a)[0])))

static int combo_add(HWND d, int id, const wchar_t *t) {
    return (int)SendDlgItemMessageW(d, id, CB_ADDSTRING, 0, (LPARAM)t);
}

static void combo_sel(HWND d, int id, int i) { SendDlgItemMessageW(d, id, CB_SETCURSEL, (WPARAM)i, 0); }
static int combo_get(HWND d, int id) { return (int)SendDlgItemMessageW(d, id, CB_GETCURSEL, 0, 0); }

static void check(HWND d, int id, int on) { CheckDlgButton(d, id, on ? BST_CHECKED : BST_UNCHECKED); }
static int checked(HWND d, int id) { return IsDlgButtonChecked(d, id) == BST_CHECKED; }

static void keys_refresh(void) {
    int i, j, sel = ListView_GetNextItem(g_keys, -1, LVNI_SELECTED);
    int p = phone_index(g_set.phone);
    ListView_DeleteAllItems(g_keys);
    for (i = 0; i < ACTION_COUNT; i++) {
        LVITEMW it;
        wchar_t keys[256] = L"", nm[64], label[96];
        for (j = 0; j < MAX_BIND; j++) {
            if (!g_set.keys[i][j]) continue;
            sym_name(g_set.keys[i][j], nm, 64);
            if (keys[0]) wcscat(keys, L",  ");
            wcscat(keys, nm);
        }
        if (Actions[i].code == ACT_SOFT1 || Actions[i].code == ACT_SOFT2)
            _snwprintf(label, 96, L"%ls  (%d)", Actions[i].label,
                       Actions[i].code == ACT_SOFT1 ? Phones[p].soft1 : Phones[p].soft2);
        else
            wcscpy(label, Actions[i].label);
        memset(&it, 0, sizeof(it));
        it.mask = LVIF_TEXT;
        it.iItem = i;
        it.pszText = label;
        ListView_InsertItem(g_keys, &it);
        ListView_SetItemText(g_keys, i, 1, keys);
    }
    if (sel >= 0) {
        ListView_SetItemState(g_keys, sel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(g_keys, sel, FALSE);
    }
}

static void pages_load(void) {
    HWND d = g_pages[1], c = g_pages[2], so = g_pages[3];
    int i;
    wchar_t w[64];
    g_loading = 1;
    combo_sel(d, IDC_SCALE, g_set.scale - 1);
    check(d, IDC_FULLSCREEN, g_set.fullscreen);
    for (i = 0; i < N(Modes); i++) if (!strcmp(Modes[i].id, g_set.mode)) combo_sel(d, IDC_MODE, i);
    for (i = 0; i < N(Filters); i++) if (!strcmp(Filters[i].id, g_set.filter)) combo_sel(d, IDC_FILTER, i);
    if (g_set.lcd[0]) {
        wchar_t *lw = widen_utf8(g_set.lcd);
        SetDlgItemTextW(d, IDC_LCD, lw);
        free(lw);
    } else {
        combo_sel(d, IDC_LCD, 0);
    }
    for (i = 0; i < N(Fpss); i++) if (!strcmp(Fpss[i].id, g_set.fps)) combo_sel(d, IDC_FPS, i);
    combo_sel(d, IDC_HEAP, 0);
    for (i = 0; i < N(Heaps); i++) if (Heaps[i] == g_set.heap) combo_sel(d, IDC_HEAP, i);
    check(d, IDC_JIT, g_set.jit);
    check(d, IDC_VERIFY, g_set.verify);
    {
        wchar_t *vw = widen_utf8(g_set.vmargs);
        SetDlgItemTextW(d, IDC_VMARGS, vw);
        free(vw);
    }
    combo_sel(c, IDC_PHONE, phone_index(g_set.phone));
    keys_refresh();
    check(so, IDC_MUTE, g_set.mute);
    SendDlgItemMessageW(so, IDC_VOLUME, TBM_SETPOS, TRUE, g_set.volume);
    _snwprintf(w, 64, L"%d%%", g_set.volume);
    SetDlgItemTextW(so, IDC_VOLTEXT, w);
    for (i = 0; i < N(Synths); i++) if (!strcmp(Synths[i].id, g_set.synth)) combo_sel(so, IDC_SYNTH, i);
    check(so, IDC_LOG, g_set.log);
    check(so, IDC_CONSOLE, g_set.console);
    check(so, IDC_LOGKEYS, g_set.log_keys);
    check(so, IDC_LOGEVENTS, g_set.log_events);
    check(so, IDC_LOGDISPLAY, g_set.log_display);
    check(so, IDC_SHOWFPS, g_set.show_fps);
    g_loading = 0;
}

static void scope_text(void) {
    wchar_t t[WBUF];
    if (g_scope_game && g_sel_jar[0])
        _snwprintf(t, WBUF, L"Editing: settings of %ls only", basename_w(g_sel_jar));
    else
        wcscpy(t, L"Editing: settings for all games");
    SetDlgItemTextW(g_main, IDC_SCOPE, t);
}

static void save_scope(void) {
    if (g_scope_game && g_sel_jar[0]) {
        wchar_t gi[WBUF];
        game_ini_path(g_sel_jar, gi);
        settings_save_file(&g_set, gi, 0);
    } else {
        settings_save_file(&g_set, g_ini, 1);
    }
}

/* the selection or the scope changed: show the right settings */
static void scope_update(void) {
    wchar_t gi[WBUF];
    int own = 0;
    if (g_sel_jar[0]) {
        game_ini_path(g_sel_jar, gi);
        own = file_exists(gi);
    }
    g_scope_game = own;
    EnableWindow(GetDlgItem(g_main, IDC_OWN), g_sel_jar[0] != 0);
    check(g_main, IDC_OWN, own);
    if (own) settings_for(&g_set, g_sel_jar);
    else {
        settings_for(&g_set, NULL);
    }
    pages_load();
    scope_text();
}

static void pages_read(void) {
    HWND d = g_pages[1], c = g_pages[2], so = g_pages[3];
    int i;
    wchar_t w[256];
    if (g_loading) return;
    g_set.scale = combo_get(d, IDC_SCALE) + 1;
    if (g_set.scale < 1) g_set.scale = 3;
    g_set.fullscreen = checked(d, IDC_FULLSCREEN);
    i = combo_get(d, IDC_MODE); if (i >= 0) copy_str(g_set.mode, sizeof(g_set.mode), Modes[i].id);
    i = combo_get(d, IDC_FILTER); if (i >= 0) copy_str(g_set.filter, sizeof(g_set.filter), Filters[i].id);
    i = combo_get(d, IDC_LCD);
    if (i >= 0) {
        SendDlgItemMessageW(d, IDC_LCD, CB_GETLBTEXT, (WPARAM)i, (LPARAM)w);
    } else {
        GetDlgItemTextW(d, IDC_LCD, w, 256);
    }
    {
        int a, b;
        g_set.lcd[0] = 0;
        if (swscanf(w, L"%dx%d", &a, &b) == 2 && a >= 64 && b >= 64 && a <= 1024 && b <= 1024)
            snprintf(g_set.lcd, sizeof(g_set.lcd), "%dx%d", a, b);
    }
    i = combo_get(d, IDC_FPS); if (i >= 0) copy_str(g_set.fps, sizeof(g_set.fps), Fpss[i].id);
    i = combo_get(d, IDC_HEAP); if (i >= 0) g_set.heap = Heaps[i];
    g_set.jit = checked(d, IDC_JIT);
    g_set.verify = checked(d, IDC_VERIFY);
    GetDlgItemTextW(d, IDC_VMARGS, w, 256);
    {
        char *u = utf8_of(w);
        copy_str(g_set.vmargs, sizeof(g_set.vmargs), u);
        free(u);
    }
    i = combo_get(c, IDC_PHONE); if (i >= 0) copy_str(g_set.phone, sizeof(g_set.phone), Phones[i].id);
    g_set.mute = checked(so, IDC_MUTE);
    g_set.volume = (int)SendDlgItemMessageW(so, IDC_VOLUME, TBM_GETPOS, 0, 0);
    i = combo_get(so, IDC_SYNTH); if (i >= 0) copy_str(g_set.synth, sizeof(g_set.synth), Synths[i].id);
    g_set.log = checked(so, IDC_LOG);
    g_set.console = checked(so, IDC_CONSOLE);
    g_set.log_keys = checked(so, IDC_LOGKEYS);
    g_set.log_events = checked(so, IDC_LOGEVENTS);
    g_set.log_display = checked(so, IDC_LOGDISPLAY);
    g_set.show_fps = checked(so, IDC_SHOWFPS);
    save_scope();
}

/* ---- game list ---- */

static int g_game_count;

static void add_game_item(const wchar_t *jar) {
    Zip z;
    wchar_t jam[WBUF], name[512] = L"", *w;
    const wchar_t *type = L"MIDP";
    LVITEMW it;
    GameItem *gi;
    int row;
    if (!zip_open(&z, jar)) return;
    find_jam(jar, &z, jam);
    if (jam[0]) {
        unsigned char *d;
        DWORD n;
        type = L"i-appli";
        if (read_file(jam, &d, &n)) {
            char *t = normalize_descriptor(d), v[512];
            if (desc_value(t, "AppName", '=', v, sizeof(v)) && v[0]) {
                w = widen_cp(v, 932);
                wcsncpy(name, w, 511);
                free(w);
            }
            free(t);
            free(d);
        }
    } else {
        DWORD n;
        unsigned char *mf = zip_read(&z, "META-INF/MANIFEST.MF", 1, &n);
        if (mf) {
            char *t = normalize_descriptor(mf), v[512];
            if (desc_value(t, "MIDlet-Name", ':', v, sizeof(v)) && v[0]) {
                w = widen_utf8(v);
                wcsncpy(name, w, 511);
                free(w);
            }
            free(t);
            free(mf);
        }
    }
    zip_close(&z);
    if (!name[0]) {
        const wchar_t *b = basename_w(jar);
        wcsncpy(name, b, 511);
        if (wcsrchr(name, L'.')) *wcsrchr(name, L'.') = 0;
    }
    gi = (GameItem *)malloc(sizeof(GameItem));
    if (gi == NULL) return;
    wcscpy(gi->path, jar);
    memset(&it, 0, sizeof(it));
    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = g_game_count;
    it.pszText = name;
    it.lParam = (LPARAM)gi;
    row = ListView_InsertItem(g_games, &it);
    if (row >= 0) {
        wchar_t rel[WBUF];
        size_t gl = wcslen(g_set.games_dir);
        wcscpy(rel, jar);
        if (_wcsnicmp(jar, g_set.games_dir, gl) == 0 && jar[gl] == L'\\') wcscpy(rel, jar + gl + 1);
        ListView_SetItemText(g_games, row, 1, (wchar_t *)type);
        ListView_SetItemText(g_games, row, 2, rel);
        g_game_count++;
    }
}

static void scan_dir(const wchar_t *dir, int depth) {
    wchar_t pat[WBUF], p[WBUF];
    WIN32_FIND_DATAW fd;
    HANDLE h;
    _snwprintf(pat, WBUF, L"%ls\\*", dir);
    h = FindFirstFileW(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.cFileName[0] == L'.') continue;
        _snwprintf(p, WBUF, L"%ls\\%ls", dir, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (depth < 3) scan_dir(p, depth + 1);
        } else {
            const wchar_t *e = wcsrchr(fd.cFileName, L'.');
            if (e && _wcsicmp(e, L".jar") == 0 && g_game_count < 5000) add_game_item(p);
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static void games_clear(void) {
    int i, n = ListView_GetItemCount(g_games);
    for (i = 0; i < n; i++) {
        LVITEMW it;
        memset(&it, 0, sizeof(it));
        it.mask = LVIF_PARAM;
        it.iItem = i;
        if (ListView_GetItem(g_games, &it) && it.lParam) free((void *)it.lParam);
    }
    ListView_DeleteAllItems(g_games);
    g_game_count = 0;
}

static void games_refresh(void) {
    wchar_t t[WBUF];
    HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));
    Settings glob;
    settings_for(&glob, NULL);
    wcscpy(g_set.games_dir, glob.games_dir);
    SendMessageW(g_games, WM_SETREDRAW, FALSE, 0);
    games_clear();
    mkdirs(glob.games_dir);
    scan_dir(glob.games_dir, 0);
    SendMessageW(g_games, WM_SETREDRAW, TRUE, 0);
    SetDlgItemTextW(g_pages[0], IDC_DIR, glob.games_dir);
    _snwprintf(t, WBUF, L"%d game%ls", g_game_count, g_game_count == 1 ? L"" : L"s");
    SetDlgItemTextW(g_pages[0], IDC_COUNT, t);
    SetCursor(old);
    g_sel_jar[0] = 0;
    scope_update();
}

static void games_selection(void) {
    int i = ListView_GetNextItem(g_games, -1, LVNI_SELECTED);
    g_sel_jar[0] = 0;
    if (i >= 0) {
        LVITEMW it;
        memset(&it, 0, sizeof(it));
        it.mask = LVIF_PARAM;
        it.iItem = i;
        if (ListView_GetItem(g_games, &it) && it.lParam) wcscpy(g_sel_jar, ((GameItem *)it.lParam)->path);
    }
    scope_update();
}

static void play_selected(void) {
    if (!g_sel_jar[0]) {
        MessageBoxW(g_main, L"Select a game in the list first (or use \"Open a game...\").",
                    APP_TITLE, MB_OK | MB_ICONINFORMATION);
        return;
    }
    run_game(g_main, g_sel_jar, 0);
}

static int pick_file(HWND owner, wchar_t *out) {
    OPENFILENAMEW of;
    memset(&of, 0, sizeof(of));
    out[0] = 0;
    of.lStructSize = sizeof(of);
    of.hwndOwner = owner;
    of.lpstrFilter = L"J2ME / i-appli games (*.jar, *.jam)\0*.jar;*.jam\0All files\0*.*\0";
    of.lpstrFile = out;
    of.nMaxFile = WBUF;
    of.lpstrTitle = L"Open a J2ME game";
    of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    return GetOpenFileNameW(&of);
}

static void pick_folder(void) {
    BROWSEINFOW bi;
    LPITEMIDLIST pidl;
    wchar_t path[MAX_PATH];
    memset(&bi, 0, sizeof(bi));
    bi.hwndOwner = g_main;
    bi.lpszTitle = L"Folder with your games (sub-folders are searched too):";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    pidl = SHBrowseForFolderW(&bi);
    if (pidl == NULL) return;
    if (SHGetPathFromIDListW(pidl, path)) {
        Settings glob;
        settings_for(&glob, NULL);
        wcsncpy(glob.games_dir, path, WBUF - 1);
        settings_save_file(&glob, g_ini, 1);
        games_refresh();
    }
    CoTaskMemFree(pidl);
}

/* ---- key capture: a plain window gets every key, also Tab/Enter/Esc ---- */

static int g_capture_sym;
static int g_capture_done;

static LRESULT CALLBACK capture_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        unsigned sc = (unsigned)((l >> 16) & 0xFF);
        int ext = (l >> 24) & 1;
        int sym = sdl_sym_of_scancode(sc, ext);
        if (w == VK_SHIFT) sym = sc == 0x36 ? K_RSHIFT : K_LSHIFT;
        if (sym > 0) {
            g_capture_sym = sym;
            g_capture_done = 1;
            DestroyWindow(h);
        }
        return 0;
    }
    case WM_SYSCHAR:
    case WM_CHAR:
        return 0;
    case WM_CLOSE:
        g_capture_done = 1;
        DestroyWindow(h);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        RECT r;
        HDC dc = BeginPaint(h, &ps);
        HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        wchar_t *text = (wchar_t *)GetWindowLongPtrW(h, GWLP_USERDATA);
        GetClientRect(h, &r);
        FillRect(dc, &r, (HBRUSH)(COLOR_WINDOW + 1));
        SelectObject(dc, f);
        SetBkMode(dc, TRANSPARENT);
        DrawTextW(dc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        EndPaint(h, &ps);
        return 0;
    }
    }
    return DefWindowProcW(h, m, w, l);
}

static int capture_key(HWND owner, const wchar_t *action) {
    static int registered;
    wchar_t text[256];
    HWND h;
    MSG msg;
    RECT r;
    int dpi = 96, W, H;
    HDC dc = GetDC(NULL);
    if (dc) { dpi = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(NULL, dc); }
    W = 340 * dpi / 96;
    H = 120 * dpi / 96;
    if (!registered) {
        WNDCLASSW wc;
        memset(&wc, 0, sizeof(wc));
        wc.lpfnWndProc = capture_proc;
        wc.hInstance = g_inst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"FunKeyCapture";
        RegisterClassW(&wc);
        registered = 1;
    }
    _snwprintf(text, 256, L"\n\nPress the PC key for:  %ls\n\n(close this window to cancel)", action);
    GetWindowRect(owner, &r);
    h = CreateWindowExW(WS_EX_DLGMODALFRAME, L"FunKeyCapture", APP_TITLE,
                        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                        (r.left + r.right - W) / 2, (r.top + r.bottom - H) / 2, W, H,
                        owner, NULL, g_inst, NULL);
    if (h == NULL) return 0;
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)text);
    InvalidateRect(h, NULL, TRUE);
    EnableWindow(owner, FALSE);
    SetFocus(h);
    g_capture_sym = 0;
    g_capture_done = 0;
    while (!g_capture_done && GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    return g_capture_sym;
}

static void add_key_to_selected(void) {
    int a = ListView_GetNextItem(g_keys, -1, LVNI_SELECTED), sym, i, j;
    if (a < 0) {
        MessageBoxW(g_main, L"Select a phone key in the list first.", APP_TITLE, MB_OK | MB_ICONINFORMATION);
        return;
    }
    sym = capture_key(g_main, Actions[a].label);
    if (sym <= 0) return;
    /* one meaning per PC key: remove it elsewhere */
    for (i = 0; i < ACTION_COUNT; i++)
        for (j = 0; j < MAX_BIND; j++)
            if (g_set.keys[i][j] == sym) g_set.keys[i][j] = 0;
    for (j = 0; j < MAX_BIND && g_set.keys[a][j]; j++) ;
    if (j == MAX_BIND) {               /* full: drop the oldest */
        for (j = 0; j < MAX_BIND - 1; j++) g_set.keys[a][j] = g_set.keys[a][j + 1];
        j = MAX_BIND - 1;
    }
    g_set.keys[a][j] = sym;
    /* compact */
    for (i = 0; i < ACTION_COUNT; i++) {
        int k = 0, tmp[MAX_BIND] = { 0 };
        for (j = 0; j < MAX_BIND; j++) if (g_set.keys[i][j]) tmp[k++] = g_set.keys[i][j];
        memcpy(g_set.keys[i], tmp, sizeof(tmp));
    }
    keys_refresh();
    save_scope();
}

/* ---- pages ---- */

static INT_PTR CALLBACK page_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_INITDIALOG: {
        /* tab page background like the tab control (visual styles) */
        typedef HRESULT (WINAPI *ETDT)(HWND, DWORD);
        HMODULE ux = LoadLibraryW(L"uxtheme.dll");
        ETDT f = ux ? (ETDT)(void *)GetProcAddress(ux, "EnableThemeDialogTexture") : NULL;
        if (f) f(h, 6 /* ETDT_ENABLETAB */);
        return TRUE;
    }
    case WM_COMMAND: {
        int id = LOWORD(w), code = HIWORD(w);
        switch (id) {
        case IDC_OPEN: {
            wchar_t f[WBUF];
            if (pick_file(g_main, f)) run_game(g_main, f, 0);
            return TRUE;
        }
        case IDC_FOLDER: pick_folder(); return TRUE;
        case IDC_REFRESH: games_refresh(); return TRUE;
        case IDC_ADDKEY: add_key_to_selected(); return TRUE;
        case IDC_CLEARKEY: {
            int a = ListView_GetNextItem(g_keys, -1, LVNI_SELECTED), j;
            if (a >= 0) {
                for (j = 0; j < MAX_BIND; j++) g_set.keys[a][j] = 0;
                keys_refresh();
                save_scope();
            }
            return TRUE;
        }
        case IDC_DEFKEYS: {
            int i, j;
            if (MessageBoxW(g_main, L"Put back the default PC keys for all phone keys?", APP_TITLE,
                            MB_YESNO | MB_ICONQUESTION) != IDYES) return TRUE;
            for (i = 0; i < ACTION_COUNT; i++)
                for (j = 0; j < MAX_BIND; j++) g_set.keys[i][j] = Actions[i].defaults[j];
            keys_refresh();
            save_scope();
            return TRUE;
        }
        case IDC_OPENLOG: {
            wchar_t lp[WBUF];
            if (!g_sel_jar[0]) {
                MessageBoxW(g_main, L"Select a game on the Games tab first.", APP_TITLE, MB_OK);
                return TRUE;
            }
            game_log_path(g_sel_jar, lp);
            if (!file_exists(lp)) {
                MessageBoxW(g_main, L"This game has no log yet (start it with \"Write a log file\" on).",
                            APP_TITLE, MB_OK);
                return TRUE;
            }
            ShellExecuteW(g_main, L"open", L"notepad.exe", lp, NULL, SW_SHOWNORMAL);
            return TRUE;
        }
        case IDC_OPENLOGDIR: {
            wchar_t lp[WBUF];
            _snwprintf(lp, WBUF, L"%ls\\logs", g_data);
            mkdirs(lp);
            ShellExecuteW(g_main, L"open", lp, NULL, NULL, SW_SHOWNORMAL);
            return TRUE;
        }
        case IDC_VMARGS:
            if (code == EN_KILLFOCUS) pages_read();
            return TRUE;
        case IDC_LCD:
            if (code == CBN_SELCHANGE || code == CBN_KILLFOCUS) {
                if (code == CBN_SELCHANGE) {
                    /* the edit text follows the list after this message */
                    PostMessageW(h, WM_COMMAND, MAKEWPARAM(IDC_LCD, CBN_KILLFOCUS), 0);
                    return TRUE;
                }
                pages_read();
            }
            return TRUE;
        case IDC_PHONE:
            if (code == CBN_SELCHANGE) { pages_read(); keys_refresh(); }
            return TRUE;
        default:
            if (code == CBN_SELCHANGE || code == BN_CLICKED) pages_read();
            return TRUE;
        }
    }
    case WM_HSCROLL:
        if ((HWND)l == GetDlgItem(h, IDC_VOLUME)) {
            wchar_t t[16];
            int v = (int)SendDlgItemMessageW(h, IDC_VOLUME, TBM_GETPOS, 0, 0);
            _snwprintf(t, 16, L"%d%%", v);
            SetDlgItemTextW(h, IDC_VOLTEXT, t);
            if (LOWORD(w) == TB_ENDTRACK || LOWORD(w) == TB_THUMBPOSITION) pages_read();
        }
        return TRUE;
    case WM_NOTIFY: {
        NMHDR *n = (NMHDR *)l;
        if (n->idFrom == IDC_GAMES) {
            if (n->code == LVN_ITEMCHANGED) {
                NMLISTVIEW *lv = (NMLISTVIEW *)l;
                if ((lv->uChanged & LVIF_STATE) && ((lv->uNewState ^ lv->uOldState) & LVIS_SELECTED))
                    games_selection();
            } else if (n->code == NM_DBLCLK || n->code == LVN_ITEMACTIVATE) {
                play_selected();
            }
        } else if (n->idFrom == IDC_KEYS) {
            if (n->code == NM_DBLCLK || n->code == LVN_ITEMACTIVATE) add_key_to_selected();
        }
        return TRUE;
    }
    }
    return FALSE;
}

static void show_page(int i) {
    int k;
    for (k = 0; k < 4; k++) ShowWindow(g_pages[k], k == i ? SW_SHOW : SW_HIDE);
}

static void lv_columns(HWND lv, const wchar_t **names, const int *dlu, int n) {
    int i;
    RECT r = { 0, 0, 4, 8 };
    MapDialogRect(GetParent(lv), &r);
    ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    for (i = 0; i < n; i++) {
        LVCOLUMNW c;
        memset(&c, 0, sizeof(c));
        c.mask = LVCF_TEXT | LVCF_WIDTH;
        c.pszText = (wchar_t *)names[i];
        c.cx = dlu[i] * r.right / 4;
        ListView_InsertColumn(lv, i, &c);
    }
}

static INT_PTR CALLBACK main_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_INITDIALOG: {
        static const int pages[4] = { IDD_GAMES, IDD_DISPLAY, IDD_CONTROLS, IDD_SOUND };
        static const wchar_t *names[4] = { L"Games", L"Display", L"Controls", L"Sound && logs" };
        RECT rc;
        int i;
        HICON ic = LoadIconW(g_inst, MAKEINTRESOURCEW(IDI_APP));
        g_main = h;
        SendMessageW(h, WM_SETICON, ICON_BIG, (LPARAM)ic);
        SendMessageW(h, WM_SETICON, ICON_SMALL, (LPARAM)ic);
        g_tab = GetDlgItem(h, IDC_TAB);
        for (i = 0; i < 4; i++) {
            TCITEMW t;
            memset(&t, 0, sizeof(t));
            t.mask = TCIF_TEXT;
            t.pszText = (wchar_t *)names[i];
            TabCtrl_InsertItem(g_tab, i, &t);
        }
        GetWindowRect(g_tab, &rc);
        MapWindowPoints(NULL, h, (POINT *)&rc, 2);
        TabCtrl_AdjustRect(g_tab, FALSE, &rc);
        for (i = 0; i < 4; i++) {
            g_pages[i] = CreateDialogParamW(g_inst, MAKEINTRESOURCEW(pages[i]), h, page_proc, 0);
            SetWindowPos(g_pages[i], HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
        }
        g_games = GetDlgItem(g_pages[0], IDC_GAMES);
        g_keys = GetDlgItem(g_pages[2], IDC_KEYS);
        {
            const wchar_t *gc[3] = { L"Game", L"Type", L"File" };
            const int gw[3] = { 130, 40, 140 };
            const wchar_t *kc[2] = { L"Phone key", L"PC keys" };
            const int kw[2] = { 120, 186 };
            lv_columns(g_games, gc, gw, 3);
            lv_columns(g_keys, kc, kw, 2);
        }
        /* combo contents */
        {
            HWND d = g_pages[1], c = g_pages[2], so = g_pages[3];
            for (i = 1; i <= 8; i++) {
                wchar_t t[64];
                _snwprintf(t, 64, i == 1 ? L"1x (phone size)" : L"%dx", i);
                combo_add(d, IDC_SCALE, t);
            }
            for (i = 0; i < N(Modes); i++) combo_add(d, IDC_MODE, Modes[i].label);
            for (i = 0; i < N(Filters); i++) combo_add(d, IDC_FILTER, Filters[i].label);
            for (i = 0; Lcds[i]; i++) combo_add(d, IDC_LCD, Lcds[i]);
            for (i = 0; i < N(Fpss); i++) combo_add(d, IDC_FPS, Fpss[i].label);
            for (i = 0; i < N(Heaps); i++) {
                wchar_t t[64];
                if (Heaps[i] == 0) wcscpy(t, L"Default (16 MB, i-appli 32)");
                else _snwprintf(t, 64, L"%d MB", Heaps[i]);
                combo_add(d, IDC_HEAP, t);
            }
            for (i = 0; i < PHONE_COUNT; i++) combo_add(c, IDC_PHONE, Phones[i].label);
            for (i = 0; i < N(Synths); i++) combo_add(so, IDC_SYNTH, Synths[i].label);
            SendDlgItemMessageW(so, IDC_VOLUME, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
            SendDlgItemMessageW(so, IDC_VOLUME, TBM_SETTICFREQ, 10, 0);
            SendDlgItemMessageW(so, IDC_VOLUME, TBM_SETPAGESIZE, 0, 10);
        }
        show_page(0);
        settings_for(&g_set, NULL);
        games_refresh();
        DragAcceptFiles(h, TRUE);
        return TRUE;
    }
    case WM_NOTIFY: {
        NMHDR *n = (NMHDR *)l;
        if (n->idFrom == IDC_TAB && n->code == TCN_SELCHANGE) show_page(TabCtrl_GetCurSel(g_tab));
        return TRUE;
    }
    case WM_DROPFILES: {
        wchar_t f[WBUF];
        if (DragQueryFileW((HDROP)w, 0, f, WBUF)) run_game(h, f, 0);
        DragFinish((HDROP)w);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case IDC_PLAY: play_selected(); return TRUE;
        case IDC_OWN:
            if (g_sel_jar[0]) {
                wchar_t gi[WBUF];
                game_ini_path(g_sel_jar, gi);
                if (checked(h, IDC_OWN)) {
                    /* start from the current (global) settings */
                    settings_save_file(&g_set, gi, 0);
                } else {
                    DeleteFileW(gi);
                }
                scope_update();
            }
            return TRUE;
        case IDCANCEL:
            /* commit an edit that still has the focus */
            pages_read();
            EndDialog(h, 0);
            return TRUE;
        }
        return FALSE;
    case WM_APP_GAME_ENDED:
        return TRUE;
    case WM_CLOSE:
        pages_read();
        EndDialog(h, 0);
        return TRUE;
    case WM_DESTROY:
        games_clear();
        return FALSE;
    }
    return FALSE;
}

/* =============================================================== start */

static int init_paths(void) {
    wchar_t exe[WBUF], tmp[WBUF];
    GetModuleFileNameW(NULL, exe, WBUF);
    dirname_w(exe, g_home);
    _snwprintf(g_ini, WBUF, L"%ls\\settings.ini", g_home);
    _snwprintf(tmp, WBUF, L"%ls\\bin\\runMidlet.exe", g_home);
    if (!file_exists(tmp)) { fail(L"bin\\runMidlet.exe is missing next to %ls.", exe); return 0; }
    g_home_a = ascii_alias(g_home);
    if (g_home_a == NULL) {
        fail(L"The runtime folder\n%ls\nhas non-Latin characters and no short (8.3) name.\n"
             L"Move it to a folder like C:\\j2me.", g_home);
        return 0;
    }
    /* data: next to the exe when writable (portable), else %LOCALAPPDATA% */
    _snwprintf(g_data, WBUF, L"%ls\\data", g_home);
    mkdirs(g_data);
    _snwprintf(tmp, WBUF, L"%ls\\.write-test", g_data);
    if (!write_file(tmp, "", 0)) {
        DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", tmp, WBUF);
        if (n == 0 || n >= WBUF) { fail(L"Cannot write to %ls", g_data); return 0; }
        _snwprintf(g_data, WBUF, L"%ls\\funkey-j2me", tmp);
        mkdirs(g_data);
        /* settings next to the data then, too */
        _snwprintf(g_ini, WBUF, L"%ls\\settings.ini", g_data);
    } else {
        DeleteFileW(tmp);
    }
    {
        char *a = ascii_alias(g_data);
        if (a == NULL) {
            fail(L"The data folder\n%ls\nhas non-Latin characters and no short (8.3) name.\n"
                 L"Move the runtime to a folder like C:\\j2me.", g_data);
            return 0;
        }
        free(a);
    }
    return 1;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, LPWSTR cmdline, int show) {
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    INITCOMMONCONTROLSEX icc;
    (void)prev; (void)cmdline; (void)show;
    g_inst = inst;
    if (!init_paths()) return 1;
    if (argc >= 2) {
        /* drag & drop / command line: start right away */
        return run_game(NULL, argv[1], 1) ? 0 : 1;
    }
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    DialogBoxParamW(inst, MAKEINTRESOURCEW(IDD_MAIN), NULL, main_proc, 0);
    CoUninitialize();
    return 0;
}
