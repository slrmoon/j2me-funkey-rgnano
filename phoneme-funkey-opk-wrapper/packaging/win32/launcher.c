/*
 * FunKeyJ2ME.exe: Windows launcher of the desktop runtime (bin\runMidlet.exe).
 * The Windows counterpart of packaging/linux-x86/run.sh.
 *
 *   FunKeyJ2ME.exe game.jar      (or drop a .jar / .jam on the exe;
 *                                 without arguments a file dialog opens)
 *
 * MIDP: the main class comes from MIDlet-1 of a .jad next to the jar or of
 * the manifest. DoJa (i-appli): a .jam next to the jar (game.jam, JAM=..., or
 * the only .jam of the folder whose AppClass is in the jar), scratchpad
 * game.sp. Settings: settings.ini next to the exe, environment variables of
 * the same name win (SCALE, LCD, MODE, FILTER, FPS, HEAP_MB, MUTE, VMARGS,
 * VERIFY, JAM, SP, CONSOLE).
 *
 * The VM opens files through the ANSI C library, so every path it gets is
 * ASCII: 8.3 short names where Windows has them, otherwise copies in the
 * data folder (the scratchpad is copied back when the game ends).
 *
 * Build: i686-w64-mingw32-gcc -O2 -municode -mwindows launcher.c -o
 *        FunKeyJ2ME.exe -lz -lcomdlg32 -lshlwapi (static zlib)
 */
#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <zlib.h>

#define APP_TITLE L"FunKey J2ME"
#define WBUF 2048

/* ---------------------------------------------------------------- utils */

static void fail(const wchar_t *fmt, ...) {
    wchar_t msg[4096];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(msg, 4095, fmt, ap);
    msg[4095] = 0;
    va_end(ap);
    MessageBoxW(NULL, msg, APP_TITLE, MB_OK | MB_ICONERROR);
    ExitProcess(1);
}

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (p == NULL) fail(L"Out of memory");
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

/* ASCII wide string -> char (caller checked is_ascii_w) */
static char *narrow(const wchar_t *w) {
    size_t n = wcslen(w), i;
    char *a = xmalloc(n + 1);
    for (i = 0; i <= n; i++) a[i] = (char)w[i];
    return a;
}

static wchar_t *widen_utf8(const char *s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    wchar_t *w = xmalloc((size_t)(n > 0 ? n : 1) * sizeof(wchar_t));
    if (n <= 0 || !MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n)) w[0] = 0;
    return w;
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
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
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

/* copy (hard link if possible) src to dst; dst replaced */
static int link_or_copy(const wchar_t *src, const wchar_t *dst) {
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

/* ------------------------------------------------------------- settings */

static wchar_t ini_path[WBUF];

/* environment variable, else settings.ini value, else def */
static void setting(const char *key, const char *def, char *out, size_t size) {
    char line[1024];
    FILE *f;
    DWORD n = GetEnvironmentVariableA(key, out, (DWORD)size);
    if (n > 0 && n < size) return;
    f = _wfopen(ini_path, L"rb");
    if (f != NULL) {
        size_t klen = strlen(key);
        while (fgets(line, sizeof(line), f)) {
            char *p = line, *e;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '#' || *p == ';' || _strnicmp(p, key, klen) != 0) continue;
            p += klen;
            while (*p == ' ' || *p == '\t') p++;
            if (*p != '=') continue;
            p++;
            while (*p == ' ' || *p == '\t') p++;
            e = p + strlen(p);
            while (e > p && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ' || e[-1] == '\t')) *--e = 0;
            if (*p == '"' && e > p + 1 && e[-1] == '"') { p++; *--e = 0; }
            fclose(f);
            strncpy(out, p, size - 1);
            out[size - 1] = 0;
            return;
        }
        fclose(f);
    }
    strncpy(out, def, size - 1);
    out[size - 1] = 0;
}

/* ------------------------------------------------------------------ jar */

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
            return z->cd_offset < z->size;
        }
    }
    return 0;
}

/* entry name compared case-insensitively when nocase; returns malloc'ed
 * contents (NUL-terminated) or NULL */
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

/* manifest/jad text: CR removed, UTF-8 BOM removed, continuation lines
 * (starting with a space) joined */
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

/* value of "Key:" (descriptor) or "Key =" (jam) into out */
static int desc_value(const char *text, const char *key, char sep, char *out, size_t size) {
    const char *p = text;
    size_t klen = strlen(key);
    while (p && *p) {
        const char *e = strchr(p, '\n');
        const char *q = p;
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

/* third field of MIDlet-1: "name, icon, class" */
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

/* ----------------------------------------------------------- command line */

static wchar_t *cmd;
static size_t cmd_len, cmd_cap;

static void cmd_raw(const wchar_t *s) {
    size_t n = wcslen(s);
    if (cmd_len + n + 1 > cmd_cap) {
        cmd_cap = (cmd_len + n + 1) * 2 + 1024;
        cmd = realloc(cmd, cmd_cap * sizeof(wchar_t));
        if (cmd == NULL) fail(L"Out of memory");
    }
    memcpy(cmd + cmd_len, s, (n + 1) * sizeof(wchar_t));
    cmd_len += n;
}

/* one argument, quoted for CommandLineToArgv / the C runtime */
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

/* split VMARGS on spaces */
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

/* -------------------------------------------------------------------- run */

static void set_env(const char *k, const char *v) {
    SetEnvironmentVariableA(k, v);
}

static void show_log_tail(const wchar_t *log, DWORD code) {
    unsigned char *d;
    DWORD n;
    wchar_t msg[6000];
    if (read_file(log, &d, &n)) {
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
    MessageBoxW(NULL, msg, APP_TITLE, MB_OK | MB_ICONWARNING);
}

static int pick_file(wchar_t *out) {
    OPENFILENAMEW of;
    memset(&of, 0, sizeof(of));
    out[0] = 0;
    of.lStructSize = sizeof(of);
    of.lpstrFilter = L"J2ME / i-appli games (*.jar, *.jam)\0*.jar;*.jam\0All files\0*.*\0";
    of.lpstrFile = out;
    of.nMaxFile = WBUF;
    of.lpstrTitle = L"Open a J2ME game";
    of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    return GetOpenFileNameW(&of);
}

/* a .jam given instead of the jar: the jar with the same name, the
 * PackageURL file, or the only jar of the folder */
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

int wmain(int argc, wchar_t **argv) {
    wchar_t exe[WBUF], home[WBUF], data[WBUF], jar[WBUF], dir[WBUF], base[WBUF];
    wchar_t rundir[WBUF], rmsdir[WBUF], logdir[WBUF], logpath[WBUF], tmp[WBUF];
    wchar_t jam[WBUF], name_w[260], *dot;
    char name[260], s[1024], app_class[512] = "";
    char *home_a, *data_a, *jar_a = NULL;
    Zip zip;
    int doja = 0, k;
    wchar_t sp_orig[WBUF] = L"", sp_copy[WBUF] = L"";
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    HANDLE logh = INVALID_HANDLE_VALUE;
    DWORD code = 0, started;
    int console;

    GetModuleFileNameW(NULL, exe, WBUF);
    dirname_w(exe, home);
    _snwprintf(ini_path, WBUF, L"%ls\\settings.ini", home);
    _snwprintf(tmp, WBUF, L"%ls\\bin\\runMidlet.exe", home);
    if (!file_exists(tmp)) fail(L"bin\\runMidlet.exe is missing next to %ls.", exe);
    home_a = ascii_alias(home);
    if (home_a == NULL)
        fail(L"The runtime folder\n%ls\nhas non-Latin characters and no short (8.3) name.\n"
             L"Move it to a folder like C:\\j2me.", home);

    /* data: next to the exe when writable (portable), else %LOCALAPPDATA% */
    _snwprintf(data, WBUF, L"%ls\\data", home);
    mkdirs(data);
    _snwprintf(tmp, WBUF, L"%ls\\.write-test", data);
    if (!write_file(tmp, "", 0)) {
        DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", tmp, WBUF);
        if (n == 0 || n >= WBUF) fail(L"Cannot write to %ls", data);
        _snwprintf(data, WBUF, L"%ls\\funkey-j2me", tmp);
        mkdirs(data);
    } else {
        DeleteFileW(tmp);
    }
    data_a = ascii_alias(data);
    if (data_a == NULL)
        fail(L"The data folder\n%ls\nhas non-Latin characters and no short (8.3) name.\n"
             L"Move the runtime to a folder like C:\\j2me.", data);

    /* the game */
    if (argc >= 2) {
        wcsncpy(jar, argv[1], WBUF - 1);
        jar[WBUF - 1] = 0;
    } else if (!pick_file(jar)) {
        return 0;
    }
    if (!GetFullPathNameW(jar, WBUF, tmp, NULL)) fail(L"Bad path: %ls", jar);
    wcscpy(jar, tmp);
    jam[0] = 0;
    dot = wcsrchr(jar, L'.');
    if (dot && _wcsicmp(dot, L".jam") == 0) {
        wcscpy(jam, jar);
        if (!jar_for_jam(jam, jar)) fail(L"No jar found for %ls", jam);
    }
    if (!file_exists(jar)) fail(L"No such file: %ls", jar);
    if (!zip_open(&zip, jar)) fail(L"%ls is not a jar (zip) file.", jar);

    dirname_w(jar, dir);
    wcscpy(base, jar);
    dot = wcsrchr(base, L'.');
    if (dot && dot > wcsrchr(base, L'\\')) *dot = 0;
    wcsncpy(name_w, basename_w(base), 259);
    name_w[259] = 0;
    if (is_ascii_w(name_w)) {
        char *n = narrow(name_w);
        strcpy(name, n);
        free(n);
    } else {
        DWORD h = 5381;
        size_t i;
        for (i = 0; jar[i]; i++) h = h * 33 + jar[i];
        for (i = 0; name_w[i] && i < 200; i++) {
            wchar_t c = name_w[i];
            name[i] = (c < 128 && (iswalnum(c) || c == L'.' || c == L'-')) ? (char)c : '_';
        }
        name[i] = 0;
        snprintf(name + strlen(name), 40, "_%lu", (unsigned long)h);
    }
    {
        wchar_t *nw = widen_utf8(name);
        _snwprintf(rundir, WBUF, L"%ls\\run\\%ls", data, nw);
        _snwprintf(rmsdir, WBUF, L"%ls\\rms\\%ls", data, nw);
        _snwprintf(logdir, WBUF, L"%ls\\logs", data);
        _snwprintf(logpath, WBUF, L"%ls\\%ls.log", logdir, nw);
        free(nw);
    }
    mkdirs(rundir);
    mkdirs(rmsdir);
    mkdirs(logdir);

    /* DoJa? JAM setting, game.jam, or the only .jam holding the AppClass */
    setting("JAM", "", s, sizeof(s));
    if (jam[0] == 0 && s[0]) {
        wchar_t *w = widen_utf8(s);
        GetFullPathNameW(w, WBUF, jam, NULL);
        free(w);
        if (!file_exists(jam)) fail(L"No such .jam: %ls", jam);
    }
    if (jam[0] == 0) {
        _snwprintf(tmp, WBUF, L"%ls.jam", base);
        if (file_exists(tmp)) wcscpy(jam, tmp);
        else {
            _snwprintf(tmp, WBUF, L"%ls.JAM", base);
            if (file_exists(tmp)) wcscpy(jam, tmp);
        }
    }
    if (jam[0] == 0) {
        WIN32_FIND_DATAW fd;
        HANDLE h;
        int count = 0;
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
                char *t = normalize_descriptor(d), cls[512], path[600];
                if (desc_value(t, "AppClass", '=', cls, sizeof(cls)) && cls[0]) {
                    char *c;
                    for (c = cls; *c; c++) if (*c == '.') *c = '/';
                    snprintf(path, sizeof(path), "%s.class", cls);
                    if (zip_has(&zip, path)) wcscpy(jam, tmp);
                }
                free(t);
                free(d);
            }
        }
    }

    /* common environment */
    set_env("MIDP_HOME", home_a);
    {
        char *r = ascii_alias(rmsdir);
        if (r == NULL) fail(L"Bad data folder %ls", rmsdir);
        set_env("PHONEME_RMS_HOME", r);
    }
    setting("SCALE", "3", s, sizeof(s)); set_env("PHONEME_WINDOW_SCALE", s);
    setting("MODE", "fit", s, sizeof(s)); set_env("PHONEME_SCALE_MODE", s);
    setting("FILTER", "", s, sizeof(s)); if (s[0]) set_env("PHONEME_SCALE_FILTER", s);
    setting("FPS", "", s, sizeof(s)); if (s[0]) set_env("PHONEME_FPS_LIMIT", s);
    setting("MUTE", "", s, sizeof(s));
    if (s[0] == 0 || s[0] == '0') set_env("PHONEME_ENABLE_AUDIO", "1");
    {
        char lcd[64];
        setting("LCD", "", lcd, sizeof(lcd));
        if (lcd[0]) { set_env("PHONEME_LCD_SIZE", lcd); set_env("PHONEME_SOURCE_SIZE", lcd); }
    }

    /* runtime command line */
    _snwprintf(tmp, WBUF, L"%ls\\bin\\runMidlet.exe", home);
    cmd_arg_w(tmp);
    setting("VERIFY", "0", s, sizeof(s));
    if (s[0] != '1') cmd_arg("-UseVerifier");
    setting("VMARGS", "", s, sizeof(s));
    cmd_args_split(s);

    if (jam[0]) {
        unsigned char *jd;
        DWORD jn, i;
        char *jt, area[64], param[1024], ver[256], spsize[64], *hex, *sp_a, *pics_a = NULL;
        char lcd[64];
        wchar_t pics[WBUF];

        doja = 1;
        if (!read_file(jam, &jd, &jn)) fail(L"Cannot read %ls", jam);
        jt = normalize_descriptor(jd);
        desc_value(jt, "AppClass", '=', app_class, sizeof(app_class));
        if (!app_class[0]) fail(L"No AppClass in %ls", jam);
        setting("HEAP_MB", "32", s, sizeof(s)); set_env("PHONEME_HEAP_MB", s);
        desc_value(jt, "DrawArea", '=', area, sizeof(area));
        if (!(area[0] >= '0' && area[0] <= '9' && strchr(area, 'x'))) strcpy(area, "240x240");
        setting("LCD", "", lcd, sizeof(lcd));
        if (!lcd[0]) { set_env("PHONEME_LCD_SIZE", area); set_env("PHONEME_SOURCE_SIZE", area); }
        hex = xmalloc((size_t)jn * 2 + 1);
        for (i = 0; i < jn; i++) sprintf(hex + i * 2, "%02x", jd[i]);
        hex[jn * 2] = 0;

        /* scratchpad: SP setting, else game.sp / game.SP; created empty */
        setting("SP", "", s, sizeof(s));
        if (s[0]) {
            wchar_t *w = widen_utf8(s);
            GetFullPathNameW(w, WBUF, sp_orig, NULL);
            free(w);
        } else {
            _snwprintf(sp_orig, WBUF, L"%ls.sp", base);
            _snwprintf(tmp, WBUF, L"%ls.SP", base);
            if (!file_exists(sp_orig) && file_exists(tmp)) wcscpy(sp_orig, tmp);
        }
        if (!file_exists(sp_orig)) write_file(sp_orig, "", 0);
        sp_a = file_exists(sp_orig) ? ascii_alias(sp_orig) : NULL;
        if (sp_a != NULL && !is_ascii_w(basename_w(sp_orig))) {
            /* temp files are named after the scratchpad: keep a plain name */
            free(sp_a);
            sp_a = NULL;
        }
        if (sp_a == NULL) {
            char *nw = name;
            wchar_t *w = widen_utf8(nw);
            _snwprintf(sp_copy, WBUF, L"%ls\\%ls.sp", rundir, w);
            free(w);
            if (file_exists(sp_orig)) CopyFileW(sp_orig, sp_copy, FALSE);
            else write_file(sp_copy, "", 0);
            sp_a = ascii_alias(sp_copy);
            if (sp_a == NULL) fail(L"Bad data folder %ls", rundir);
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
            if (!link_or_copy(jar, tmp)) fail(L"Cannot copy %ls", jar);
            jar_a = ascii_alias(tmp);
        }

        desc_value(jt, "AppParam", '=', param, sizeof(param));
        desc_value(jt, "AppVer", '=', ver, sizeof(ver));
        desc_value(jt, "SPsize", '=', spsize, sizeof(spsize));
        /* non-ASCII (SJIS) values travel in doja.jam.hex only */
        if (!is_ascii_a(param)) param[0] = 0;
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
        {
            size_t n = strlen(jar_a) + strlen(home_a) + 64;
            char *cp = xmalloc(n);
            snprintf(cp, n, "%s;%s\\lib\\doja-support.jar", jar_a, home_a);
            cmd_arg(cp);
            free(cp);
        }
        cmd_arg("internal");
        cmd_arg("com.funkey.doja.DoJaMIDlet");
        free(hex);
        free(jt);
        free(jd);
    } else {
        /* MIDP: the game's .jad when it has MIDlet-1 (getAppProperty reads
         * the .jad next to the jar), else one made from the manifest */
        unsigned char *d = NULL, *mf;
        DWORD n;
        char *desc = NULL, main_class[512] = "";
        int own_jad = 0;

        setting("HEAP_MB", "16", s, sizeof(s)); set_env("PHONEME_HEAP_MB", s);
        _snwprintf(tmp, WBUF, L"%ls.jad", base);
        if (read_file(tmp, &d, &n)) {
            desc = normalize_descriptor(d);
            free(d);
            if (midlet_class(desc, main_class, sizeof(main_class))) own_jad = 1;
            else { free(desc); desc = NULL; }
        }
        mf = zip_read(&zip, "META-INF/MANIFEST.MF", 1, &n);
        if (!own_jad) {
            if (mf == NULL) fail(L"%ls has no manifest.", jar);
            desc = normalize_descriptor(mf);
            midlet_class(desc, main_class, sizeof(main_class));
        } else if (!main_class[0] && mf) {
            char *m = normalize_descriptor(mf);
            midlet_class(m, main_class, sizeof(main_class));
            free(m);
        }
        if (!main_class[0]) fail(L"No MIDlet-1 entry in %ls", jar);

        jar_a = own_jad ? ascii_alias(jar) : NULL;
        if (jar_a != NULL) {
            /* the .jad must also be reachable by the same (short) base name */
            char jad_check[WBUF];
            snprintf(jad_check, sizeof(jad_check), "%s", jar_a);
            if (strlen(jad_check) > 4) strcpy(jad_check + strlen(jad_check) - 4, ".jad");
            if (GetFileAttributesA(jad_check) == INVALID_FILE_ATTRIBUTES) {
                free(jar_a);
                jar_a = NULL;
            }
        }
        if (jar_a == NULL) {
            _snwprintf(tmp, WBUF, L"%ls\\game.jar", rundir);
            if (!link_or_copy(jar, tmp)) fail(L"Cannot copy %ls", jar);
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

    /* start: log to data\logs\<game>.log, no console unless CONSOLE=1 */
    setting("CONSOLE", "0", s, sizeof(s));
    console = s[0] == '1';
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));
    if (!console) {
        memset(&sa, 0, sizeof(sa));
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        logh = CreateFileW(logpath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (logh != INVALID_HANDLE_VALUE) {
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            si.hStdOutput = logh;
            si.hStdError = logh;
        }
    }
    {
        wchar_t bindir[WBUF];
        _snwprintf(bindir, WBUF, L"%ls\\bin", home);
        if (!CreateProcessW(NULL, cmd, NULL, NULL, TRUE,
                            console ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW,
                            NULL, bindir, &si, &pi))
            fail(L"Cannot start bin\\runMidlet.exe (error %lu).", (unsigned long)GetLastError());
    }
    started = GetTickCount();
    if (logh != INVALID_HANDLE_VALUE) CloseHandle(logh);
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);

    /* scratchpad copy back */
    if (sp_copy[0] && sp_orig[0]) {
        if (!CopyFileW(sp_copy, sp_orig, FALSE))
            MessageBoxW(NULL, L"Could not write the scratchpad (save) back to the game folder.",
                        APP_TITLE, MB_OK | MB_ICONWARNING);
    }
    k = (int)code;
    if (code != 0 && !console) show_log_tail(logpath, code);
    (void)started;
    (void)doja;
    return k;
}
