/*
 * Browser build (Emscripten): SDL 1.2 surfaces in plain memory.
 *
 * Emscripten's SDL keeps every surface 32-bit (a canvas behind it), while
 * this port draws RGB565 straight into surface pixels everywhere. So in the
 * browser the surface functions the port uses are implemented here, in C,
 * over ordinary memory (C definitions take precedence over Emscripten's
 * JavaScript library ones), and SDL_Flip() of the video surface converts it
 * to RGBA and puts it on the page canvas. Events, timers and audio stay
 * Emscripten's SDL.
 *
 * characterColor() (SDL_gfx, runtime menu text) is here too: SDL_gfx draws
 * glyphs by blitting 32-bit surfaces.
 */
#ifdef __EMSCRIPTEN__

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <emscripten.h>
#include <SDL.h>
#include <SDL_gfxPrimitives_font.h>

static SDL_Surface *VideoSurface;

static int mask_shift(Uint32 m) {
    int s = 0;
    if (m == 0) return 0;
    while (!(m & 1)) { m >>= 1; s++; }
    return s;
}

static int mask_loss(Uint32 m) {
    int b = 0;
    if (m == 0) return 8;
    m >>= mask_shift(m);
    while (m & 1) { m >>= 1; b++; }
    return 8 - b;
}

static SDL_PixelFormat *new_format(int depth, Uint32 r, Uint32 g, Uint32 b, Uint32 a) {
    SDL_PixelFormat *f = (SDL_PixelFormat *) calloc(1, sizeof(SDL_PixelFormat));
    if (f == NULL) return NULL;
    if (depth == 16 && r == 0 && g == 0 && b == 0) { r = 0xF800; g = 0x07E0; b = 0x001F; }
    if (depth == 32 && r == 0 && g == 0 && b == 0) { r = 0xFF0000; g = 0x00FF00; b = 0x0000FF; }
    f->BitsPerPixel = (Uint8) depth;
    f->BytesPerPixel = (Uint8) ((depth + 7) / 8);
    f->Rmask = r; f->Gmask = g; f->Bmask = b; f->Amask = a;
    f->Rshift = (Uint8) mask_shift(r); f->Rloss = (Uint8) mask_loss(r);
    f->Gshift = (Uint8) mask_shift(g); f->Gloss = (Uint8) mask_loss(g);
    f->Bshift = (Uint8) mask_shift(b); f->Bloss = (Uint8) mask_loss(b);
    f->Ashift = (Uint8) mask_shift(a); f->Aloss = (Uint8) mask_loss(a);
    f->refcount = 1;
    return f;
}

static SDL_Surface *new_surface(int w, int h, int depth, void *pixels, int pitch,
                                Uint32 r, Uint32 g, Uint32 b, Uint32 a) {
    SDL_Surface *s;
    if (w <= 0 || h <= 0 || (depth != 8 && depth != 16 && depth != 24 && depth != 32)) {
        return NULL;
    }
    s = (SDL_Surface *) calloc(1, sizeof(SDL_Surface));
    if (s == NULL) return NULL;
    s->format = new_format(depth, r, g, b, a);
    s->w = w;
    s->h = h;
    if (pixels != NULL) {
        s->pixels = pixels;
        s->pitch = pitch;
        s->flags = SDL_PREALLOC;
    } else {
        s->pitch = (w * s->format->BytesPerPixel + 3) & ~3;
        s->pixels = calloc((size_t) h, (size_t) s->pitch);
    }
    if (s->format == NULL || s->pixels == NULL) {
        free(s->format);
        free(s);
        return NULL;
    }
    s->clip_rect.w = w;
    s->clip_rect.h = h;
    s->refcount = 1;
    return s;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth,
                                  Uint32 r, Uint32 g, Uint32 b, Uint32 a) {
    (void) flags;
    return new_surface(w, h, depth, NULL, 0, r, g, b, a);
}

SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int w, int h, int depth, int pitch,
                                      Uint32 r, Uint32 g, Uint32 b, Uint32 a) {
    return new_surface(w, h, depth, pixels, pitch, r, g, b, a);
}

void SDL_FreeSurface(SDL_Surface *s) {
    if (s == NULL || s == VideoSurface) return;
    if (!(s->flags & SDL_PREALLOC)) free(s->pixels);
    free(s->format);
    free(s);
}

int SDL_LockSurface(SDL_Surface *s) { (void) s; return 0; }
void SDL_UnlockSurface(SDL_Surface *s) { (void) s; }

Uint32 SDL_MapRGBA(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    return ((Uint32) (r >> f->Rloss) << f->Rshift) | ((Uint32) (g >> f->Gloss) << f->Gshift)
         | ((Uint32) (b >> f->Bloss) << f->Bshift)
         | (f->Amask ? ((Uint32) (a >> f->Aloss) << f->Ashift) : 0);
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b) {
    return SDL_MapRGBA(f, r, g, b, 255);
}

void SDL_GetRGB(Uint32 p, const SDL_PixelFormat *f, Uint8 *r, Uint8 *g, Uint8 *b) {
    *r = (Uint8) (((p & f->Rmask) >> f->Rshift) << f->Rloss);
    *g = (Uint8) (((p & f->Gmask) >> f->Gshift) << f->Gloss);
    *b = (Uint8) (((p & f->Bmask) >> f->Bshift) << f->Bloss);
}

static void put_pixel(SDL_Surface *s, int x, int y, Uint32 c) {
    Uint8 *p = (Uint8 *) s->pixels + y * s->pitch + x * s->format->BytesPerPixel;
    switch (s->format->BytesPerPixel) {
    case 1: *p = (Uint8) c; break;
    case 2: *(Uint16 *) p = (Uint16) c; break;
    case 3: p[0] = (Uint8) c; p[1] = (Uint8) (c >> 8); p[2] = (Uint8) (c >> 16); break;
    default: *(Uint32 *) p = c; break;
    }
}

int SDL_FillRect(SDL_Surface *s, const SDL_Rect *rect, Uint32 color) {
    int x0 = 0, y0 = 0, x1, y1, x, y;
    if (s == NULL) return -1;
    x1 = s->w;
    y1 = s->h;
    if (rect != NULL) {
        x0 = rect->x; y0 = rect->y;
        x1 = rect->x + rect->w; y1 = rect->y + rect->h;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > s->w) x1 = s->w;
        if (y1 > s->h) y1 = s->h;
    }
    for (y = y0; y < y1; y++) {
        if (s->format->BytesPerPixel == 2) {
            Uint16 *p = (Uint16 *) ((Uint8 *) s->pixels + y * s->pitch);
            for (x = x0; x < x1; x++) p[x] = (Uint16) color;
        } else {
            for (x = x0; x < x1; x++) put_pixel(s, x, y, color);
        }
    }
    return 0;
}

SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, Uint32 flags) {
    (void) bpp;
    (void) flags;
    if (VideoSurface != NULL && VideoSurface->w == w && VideoSurface->h == h) {
        return VideoSurface;
    }
    if (VideoSurface != NULL) {
        SDL_Surface *old = VideoSurface;
        VideoSurface = NULL;
        SDL_FreeSurface(old);
    }
    VideoSurface = new_surface(w, h, 16, NULL, 0, 0xF800, 0x07E0, 0x001F, 0);
    return VideoSurface;
}

SDL_Surface *SDL_GetVideoSurface(void) { return VideoSurface; }

/* Emscripten's SDL_ShowCursor goes through pointer lock (which a page
 * gets only after a click); the cursor over the game area stays as it is */
int SDL_ShowCursor(int toggle) {
    static int shown = 1;
    if (toggle >= 0) shown = toggle;
    return shown;
}

/* the page's game area changed size (window, full screen): the desktop
 * presenter takes it as a window resize and draws at the new size */
EMSCRIPTEN_KEEPALIVE void fk_web_resize(int w, int h) {
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = SDL_VIDEORESIZE;
    e.resize.w = w;
    e.resize.h = h;
    SDL_PushEvent(&e);
}

/* back to the browser for a moment (MessageChannel: no 4 ms timer clamp) */
EM_ASYNC_JS(void, fk_web_yield_js, (void), {
    await new Promise(function (resolve) {
        var c = new MessageChannel();
        c.port1.onmessage = function () { resolve(); };
        c.port2.postMessage(0);
    });
});

/* (an EM_ASYNC_JS function is an import of this file only) */
void fk_web_yield(void) { fk_web_yield_js(); }

/* Wait up to ms, but come back as soon as an input event is queued for
 * SDL (key, pointer, the page's on-screen pad): a game waiting between its
 * frames saw a key only at the end of a 16 ms step before, up to 17 ms
 * after it was pressed. */
EM_ASYNC_JS(void, fk_web_wait_js, (int ms), {
    if (typeof SDL !== 'undefined' && SDL.events && !SDL.events.fkWakes) {
        var q = SDL.events, push = q.push;
        q.fkWakes = true;
        q.push = function () {
            var r = push.apply(this, arguments);
            var w = Module.fkWake;
            if (w) { Module.fkWake = null; w(); }
            return r;
        };
    }
    await new Promise(function (resolve) {
        var done = false;
        var finish = function () {
            if (done) return;
            done = true;
            Module.fkWake = null;
            clearTimeout(t);
            resolve();
        };
        var t = setTimeout(finish, ms);
        if (typeof SDL !== 'undefined' && SDL.events && SDL.events.length) { finish(); return; }
        /* after the event that queued it has been dispatched in full */
        Module.fkWake = function () {
            var c = new MessageChannel();
            c.port1.onmessage = finish;
            c.port2.postMessage(0);
        };
    });
});

void fk_web_wait(int ms) { fk_web_wait_js(ms); }

/*
 * Frame pacing. A browser shows a canvas at the next display refresh. A game
 * that draws every 33 or 50 ms (2 or 3 refreshes at 60 Hz) with a
 * millisecond of jitter, and whose frames happen to come just before a
 * refresh, has some frames shown one refresh early or late: uneven motion
 * at a steady frame rate. A window on the desktop has no such grid.
 * So when a game draws slower than the display, each frame is put on the
 * canvas from requestAnimationFrame, moved later by up to one refresh so
 * that frames come, on average, half a refresh before one: a millisecond
 * or two of jitter then no longer changes the refresh a frame lands on.
 * Off by default (Debug dialog: Frame pacing, Module.fkPaceOn): it costs up
 * to a refresh of delay. Faster games and hidden pages: drawn at once.
 */
EM_JS(void, fk_web_present, (const void *rgba, int w, int h), {
    var P = Module.fkPace;
    if (!P) {
        P = Module.fkPace = { v: 1000 / 60, last: 0, gaps: [], c: 0, s: 0, frame: 0, prev: 0,
                              pending: null, on: !!Module.fkPaceOn && !/[?&]pace=0/.test(location.search) };
        var draw = function (f) {
            var c = Module['canvas'];
            if (!c) return;
            if (c.width !== f.w || c.height !== f.h) { c.width = f.w; c.height = f.h; Module.fkCtx = null; }
            /* low latency (Debug dialog, on by default): the canvas goes to
             * the screen without waiting for the page's compositor - a
             * frame or two less delay, as with a desktop window. Opaque:
             * nothing to blend. Fixed for a canvas at its first use. */
            if (!Module.fkCtx) Module.fkCtx = c.getContext('2d', { alpha: false,
                desynchronized: !Module.fkNoLowLatency });
            Module.fkCtx.putImageData(new ImageData(new Uint8ClampedArray(HEAPU8.buffer, f.p, f.w * f.h * 4), f.w, f.h), 0, 0);
        };
        P.draw = draw;
        var tick = function () {
            var t = performance.now();
            if (P.last) {
                var g = t - P.last;
                if (g > 4 && g < 60) { P.gaps.push(g); if (P.gaps.length > 40) P.gaps.shift(); }
                if (P.gaps.length >= 10 && (P.gaps.length & 7) === 0) {
                    var q = P.gaps.slice().sort(function (a, b) { return a - b; });
                    P.v = q[q.length >> 1];
                }
            }
            P.last = t;
            if (P.pending && t >= P.pending.due) { var f = P.pending; P.pending = null; draw(f); }
            requestAnimationFrame(tick);
        };
        requestAnimationFrame(tick);
    }
    var now = performance.now(), f = { p: rgba, w: w, h: h, due: now };
    Module.fkFrames = (Module.fkFrames || 0) + 1;
    if (P.prev) P.frame = P.frame ? P.frame * 0.9 + (now - P.prev) * 0.1 : now - P.prev;
    P.prev = now;
    /* the SDL_Flip buffer is the same for every frame: a frame still
     * waiting is replaced by the newer one, never drawn after it */
    if (!P.on || !P.last || now - P.last > 100 || P.frame < P.v * 1.5) {
        P.pending = null;
        P.draw(f);
        return;
    }
    /* phase of this frame between refreshes, averaged over recent frames */
    var a = 2 * Math.PI * (((now - P.last) % P.v) / P.v);
    P.c = P.c * 0.9 + Math.cos(a); P.s = P.s * 0.9 + Math.sin(a);
    var mean = Math.atan2(P.s, P.c) / (2 * Math.PI);
    var shift = ((0.5 - mean) % 1 + 1) % 1;
    f.due = now + shift * P.v - 1;
    P.pending = f;
});

int SDL_Flip(SDL_Surface *s) {
    static Uint32 *rgba, *lut;
    static int size;
    int x, y;
    if (s == NULL || s != VideoSurface) return 0;
    if (lut == NULL) {
        lut = (Uint32 *) malloc(65536 * sizeof(Uint32));
        if (lut == NULL) return -1;
        for (x = 0; x < 65536; x++) {
            Uint32 r = (x >> 11) & 31, g = (x >> 5) & 63, b = x & 31;
            r = (r << 3) | (r >> 2); g = (g << 2) | (g >> 4); b = (b << 3) | (b >> 2);
            lut[x] = 0xFF000000u | (b << 16) | (g << 8) | r;
        }
    }
    if (size != s->w * s->h) {
        free(rgba);
        size = s->w * s->h;
        rgba = (Uint32 *) malloc((size_t) size * sizeof(Uint32));
        if (rgba == NULL) { size = 0; return -1; }
    }
    for (y = 0; y < s->h; y++) {
        const Uint16 *src = (const Uint16 *) ((const Uint8 *) s->pixels + y * s->pitch);
        Uint32 *dst = rgba + y * s->w;
        for (x = 0; x < s->w; x++) dst[x] = lut[src[x]];
    }
    fk_web_present(rgba, s->w, s->h);
    return 0;
}

void SDL_UpdateRect(SDL_Surface *s, Sint32 x, Sint32 y, Uint32 w, Uint32 h) {
    (void) x; (void) y; (void) w; (void) h;
    SDL_Flip(s);
}

/* SDL_gfx's 8x8 font, drawn straight into the surface (runtime menu) */
int characterColor(SDL_Surface *dst, Sint16 x, Sint16 y, char c, Uint32 color) {
    const unsigned char *g = gfxPrimitivesFontdata + (unsigned char) c * 8;
    Uint32 pix = SDL_MapRGBA(dst->format, (Uint8) (color >> 24), (Uint8) (color >> 16),
                             (Uint8) (color >> 8), (Uint8) color);
    int row, col;
    for (row = 0; row < 8; row++) {
        int yy = y + row;
        if (yy < 0 || yy >= dst->h) continue;
        for (col = 0; col < 8; col++) {
            int xx = x + col;
            if (xx < 0 || xx >= dst->w) continue;
            if (g[row] & (0x80 >> col)) put_pixel(dst, xx, yy, pix);
        }
    }
    return 0;
}

#endif /* __EMSCRIPTEN__ */
