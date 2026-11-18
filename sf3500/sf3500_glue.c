#include "sf3500.h"
#include "SDL_ttf.h"
#include "hwdisp.hpp"
#include "joykey.hpp"
#include "config.h"
#include "sfsound.h"
#include "palette8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

static SDL_Surface* g_surf;      /* the whole 640x480 screen */
static SDL_Surface* g_view;      /* the game's window inside it (video modes below) */
static SDL_Renderer* g_ren;
static int g_fps = 40, g_headless, g_shot_every, g_maxframes;
static unsigned g_frame, g_last_ticks;
static const char* g_shots;
static unsigned short g_lut[65536];          /* RGB565 -> TI-92 LCD shade */
static int g_look, g_vx, g_vy, g_vw, g_vh;   /* look filter on, and the game's window rect */

/* TI-92 look: the calculator shows four grays on a green-gray LCD, with a dark bezel around. */
#define LCD(r, g, b) ((unsigned short)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
static void build_look_lut(void) {
    static const unsigned short shade[4] = { LCD(190, 204, 180), LCD(146, 164, 140), LCD(84, 102, 92), LCD(26, 36, 38) };
    int i;
    for (i = 0; i < 65536; i++) {
        int r = ((i >> 11) & 31) * 255 / 31, g = ((i >> 5) & 63) * 255 / 63, b = (i & 31) * 255 / 31;
        int l = (r * 299 + g * 587 + b * 114) / 1000;
        g_lut[i] = shade[l >= 205 ? 0 : (l >= 140 ? 1 : (l >= 70 ? 2 : 3))];
    }
}

static void apply_look(void) {
    int y, x;
    for (y = g_vy; y < g_vy + g_vh; y++) {
        unsigned short* p = (unsigned short*)((unsigned char*)g_surf->pixels + y * g_surf->pitch) + g_vx;
        for (x = 0; x < g_vw; x++) p[x] = g_lut[p[x]];
    }
}
static char g_script[1024];

/* Video modes: the game sees view_w x view_h pixels drawn scale times bigger, in a window
 * of view*scale centered on the 640x480 screen; the rest stays black (pillarbox/letterbox),
 * like the original's window on a desktop. */
static const struct { int vw, vh, scale; } k_modes[CFG_NMODES] = {
    { 640, 480, 1 },   /* fills the screen */
    { 480, 360, 1 },
    { 320, 240, 1 },
    { 320, 240, 2 },   /* fills the screen */
    { 256, 192, 2 },
};

void sf3500_mode_info(int mode, int* vw, int* vh, int* scale, int* ww, int* wh) {
    if (mode < 0 || mode >= CFG_NMODES) mode = 0;
    *vw = k_modes[mode].vw; *vh = k_modes[mode].vh; *scale = k_modes[mode].scale;
    *ww = *vw * *scale; *wh = *vh * *scale;
}

static int env_int(const char* name, int def) {
    const char* v = getenv(name);
    return (v && *v) ? atoi(v) : def;
}

int sf3500_start(void) {
    const char* s;
    g_fps = g_cfg.fps;
    g_headless = env_int("SKI_HEADLESS", 0);
    g_shots = getenv("SKI_SHOTS");
    g_shot_every = env_int("SKI_SHOT_EVERY", 0);
    g_maxframes = env_int("SKI_MAXFRAMES", 0);
    s = getenv("SKI_JOY");
    if (s) snprintf(g_script, sizeof(g_script), "%s", s);

    /* the card's background scripts wake up often; a better priority keeps frames even
     * (Water Closet: late frames 14-19 per 2 s -> 0 with nice -10) */
    if (g_cfg.nice) setpriority(PRIO_PROCESS, 0, g_cfg.nice);

    if (!g_headless) {
        if (hwdisp_init() != 0) {
            fprintf(stderr, "sf3500: hwdisp_init failed\n");
            return -1;
        }
    }
    joykey_init();
    g_surf = SDL_CreateRGBSurfaceWithFormat(0, SF_W, SF_H, 16, SDL_PIXELFORMAT_RGB565);
    if (!g_surf) {
        fprintf(stderr, "sf3500: surface: %s\n", SDL_GetError());
        return -1;
    }
    fprintf(stderr, "sf3500: fps cap %d%s\n", g_fps, g_headless ? ", headless" : "");
    return 0;
}

unsigned short* sf3500_pixels(int* stride_px) {
    if (stride_px) *stride_px = g_surf->pitch / 2;
    return (unsigned short*)g_surf->pixels;
}

void sf3500_set_fps(int fps) {
    g_fps = fps < 5 ? 5 : (fps > 120 ? 120 : fps);
}

/* 8-bit theme: one fixed palette (palette8.h, generated with the sprites). Every pixel of the frame is snapped to
 * its nearest entry through a 64K lookup (RGB565 -> RGB565), so art, ground, text and menus all agree. */
/* 4096 entries (RGB444 bins, 8 KB: stays in the CPU cache). Every palette colour maps to itself. */
static unsigned short g_pal_lut[4096];
static int g_pal_ready;
#define PAL_IDX(v) ((((v) >> 12) << 8) | ((((v) >> 7) & 15) << 4) | (((v) >> 1) & 15))

static void build_pal_lut(void) {
    int ri, gi, bi, k;
    if (g_pal_ready) return;
    for (ri = 0; ri < 16; ri++)
        for (gi = 0; gi < 16; gi++)
            for (bi = 0; bi < 16; bi++) {
                int r = ((ri << 1) | 1) * 255 / 31, g = ((gi << 2) | 2) * 255 / 63, b = ((bi << 1) | 1) * 255 / 31;
                int best = 0, bd = 1 << 30;
                for (k = 0; k < PAL8_N; k++) {
                    int dr = r - k_pal8[k][0], dg = g - k_pal8[k][1], db = b - k_pal8[k][2];
                    int d = 2 * dr * dr + 4 * dg * dg + 3 * db * db;
                    if (d < bd) { bd = d; best = k; }
                }
                g_pal_lut[(ri << 8) | (gi << 4) | bi] = (unsigned short)(((k_pal8[best][0] >> 3) << 11) | ((k_pal8[best][1] >> 2) << 5) | (k_pal8[best][2] >> 3));
            }
    g_pal_ready = 1;
}

unsigned short sf3500_pal8(unsigned short c) { build_pal_lut(); return g_pal_lut[PAL_IDX(c)]; }

/* snaps a rectangle of the 640x480 screen to the palette */
void sf3500_posterize(int x0, int y0, int w, int h) {
    int y, x;
    build_pal_lut();
    for (y = y0; y < y0 + h && y < SF_H; y++) {
        unsigned short* p = (unsigned short*)((unsigned char*)g_surf->pixels + y * g_surf->pitch) + x0;
        for (x = 0; x < w && x0 + x < SF_W; x++) p[x] = g_pal_lut[PAL_IDX(p[x])];
    }
}

/* 16-bit theme: a tiling snow texture (soft blue-white with a few drifts of darker flecks) under the sprites,
 * scrolled with the skier so it reads as ground. The tile is kept in the screen's own RGB565 format and
 * copied row by row straight into the view: it replaces the game's per-frame clear instead of adding to it
 * (drawing it as SDL textures cost ~30x the Windows theme's frame time). */
#define SNOW_TILE 128
static unsigned short* g_tile;     /* g_tile_n x g_tile_n, RGB565, each logical pixel repeated scale x scale */
static int g_post;                 /* 8-bit theme: snap the status box corner to the palette (everything else is exact) */
static int g_vscale = 1;
static int g_tile_n;

static unsigned snow_hash(unsigned x, unsigned y) {
    unsigned h = x * 374761393u + y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static void make_snow(int scale, int block) {   /* block: 1 = fine texture (16-bit), 2 = chunky 2x2 (8-bit) */
    int x, y, i, j;
    free(g_tile);
    g_tile_n = SNOW_TILE * scale;
    g_tile = (unsigned short*)malloc((size_t)g_tile_n * g_tile_n * sizeof(unsigned short));
    if (!g_tile) return;
    for (y = 0; y < SNOW_TILE; y++)
        for (x = 0; x < SNOW_TILE; x++) {
            unsigned h = snow_hash((unsigned)(x / block), (unsigned)(y / block));
            int r = 240, g = 246, b = 254;                       /* base: snow with a blue tint */
            unsigned short c;
            if (h % 61 == 0) { r = 214; g = 226; b = 248; }      /* small shadow flecks */
            else if (h % 97 == 0) { r = 252; g = 254; b = 255; } /* glints (never pure white: that is the sprites' key) */
            else if ((x / 2 + y / 2) % 2 && snow_hash((unsigned)(x / 8), (unsigned)(y / 8)) % 5 == 0) { r = 232; g = 240; b = 252; } /* faint dithered patches */
            c = (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
            if (block > 1) c = sf3500_pal8(c);                  /* 8-bit theme: the tile is palette-exact, no per-frame pass */
            for (j = 0; j < scale; j++)
                for (i = 0; i < scale; i++) g_tile[(y * scale + j) * g_tile_n + x * scale + i] = c;
        }
}

/* Paints the whole view with the ground. Returns 0 when the theme is off (the caller clears as usual). */
int sf3500_draw_ground(int scroll_x, int scroll_y) {
    int vw, vh, sc, ww, wh, ox, oy, y;
    if (g_cfg.look < 2 || !g_tile || !g_view) return 0;
    sf3500_mode_info(g_cfg.mode, &vw, &vh, &sc, &ww, &wh);
    /* the world moves opposite to the skier: tile pixel (x - ox, y - oy) lands on screen pixel (x, y) */
    ox = (((scroll_x % SNOW_TILE) + SNOW_TILE) % SNOW_TILE) * sc;
    oy = (((scroll_y % SNOW_TILE) + SNOW_TILE) % SNOW_TILE) * sc;
    for (y = 0; y < wh; y++) {
        unsigned short* dst = (unsigned short*)((unsigned char*)g_view->pixels + y * g_view->pitch);
        const unsigned short* row = g_tile + (size_t)((((y - oy) % g_tile_n) + g_tile_n) % g_tile_n) * g_tile_n;
        int sx = (g_tile_n - ox) % g_tile_n, left = ww;
        while (left > 0) {
            int n = g_tile_n - sx;
            if (n > left) n = left;
            memcpy(dst, row + sx, (size_t)n * sizeof(unsigned short));
            dst += n; left -= n; sx = 0;
        }
    }
    return 1;
}

/* Software renderer on the game's window inside the screen, picked by g_cfg.mode. */
SDL_Renderer* sf3500_make_renderer(void) {
    int vw, vh, scale, ww, wh, x0, y0;
    sf3500_mode_info(g_cfg.mode, &vw, &vh, &scale, &ww, &wh);
    x0 = ((SF_W - ww) / 2) & ~1;      /* even: keeps 32-bit blits aligned */
    y0 = (SF_H - wh) / 2;
    g_look = (g_cfg.look == 1);
    g_post = (g_cfg.look == 3);
    g_vscale = scale;
    g_vx = x0; g_vy = y0; g_vw = ww; g_vh = wh;
    if (g_look) build_look_lut();
    SDL_FillRect(g_surf, NULL, g_look ? LCD(26, 36, 38) : 0);
    g_view = SDL_CreateRGBSurfaceWithFormatFrom((unsigned char*)g_surf->pixels + y0 * g_surf->pitch + x0 * 2,
                                                ww, wh, 16, g_surf->pitch, SDL_PIXELFORMAT_RGB565);
    if (!g_view) return NULL;
    g_ren = SDL_CreateSoftwareRenderer(g_view);
    if (g_ren && scale > 1) SDL_RenderSetLogicalSize(g_ren, vw, vh);
    if (g_cfg.look >= 2) make_snow(scale, g_cfg.look == 3 ? 2 : 1);
    fprintf(stderr, "sf3500: screen mode %d: view %dx%d, scale %d, window %dx%d at %d,%d\n", g_cfg.mode, vw, vh, scale, ww, wh, x0, y0);
    return g_ren;
}

void sf3500_output_size(int* w, int* h) {
    int scale, ww, wh;
    sf3500_mode_info(g_cfg.mode, w, h, &scale, &ww, &wh);
}

static void save_shot(void) {
    char path[512];
    snprintf(path, sizeof(path), "%s/frame%06u.bmp", g_shots, g_frame);
    SDL_SaveBMP(g_surf, path);
}

void sf3500_present(void) {
    unsigned now, target;
    if (g_look) apply_look();
    if (g_post) {   /* art and ground are palette-exact; only the status box text still needs snapping */
        int pw = 170 * g_vscale, ph = 60 * g_vscale;
        if (pw > g_vw) pw = g_vw;
        if (ph > g_vh) ph = g_vh;
        sf3500_posterize(g_vx + g_vw - pw, g_vy, pw, ph);
    }
    if (!g_headless)
        hwdisp_present(g_surf->pixels, SF_W, SF_H, g_surf->pitch);
    g_frame++;
    if (g_shots && g_shot_every > 0 && g_frame % (unsigned)g_shot_every == 0) save_shot();
    if (g_maxframes && g_frame >= (unsigned)g_maxframes) sf3500_quit(0);

    /* frame cap: sleep away what is left of this frame's time slot */
    now = SDL_GetTicks();
    target = g_last_ticks + 1000u / (unsigned)g_fps;
    if (!g_headless && now < target) SDL_Delay(target - now);
    g_last_ticks = SDL_GetTicks();
}

/* Scripted pad for off-device tests: "frame:mask,frame:mask": from that frame on the
 * pad reads as mask (hex or decimal) until the next entry. */
static unsigned scripted(void) {
    static unsigned cur;
    static const char* p;
    static int init;
    if (!init) { init = 1; p = g_script; }
    while (p && *p) {
        char* end;
        unsigned long fr = strtoul(p, &end, 10);
        unsigned long mask;
        if (*end != ':' || fr > g_frame) break;
        mask = strtoul(end + 1, &end, 0);
        cur = (unsigned)mask;
        p = (*end == ',') ? end + 1 : end;
    }
    return cur;
}

unsigned sf3500_buttons(void) {
    unsigned jk;
    if (g_script[0]) return scripted();
    jk = joykey_read();
    /* joykey's JK_* order is the SB_* order above */
    return jk;
}

void sf3500_quit(int code) {
    sfsnd_shutdown();
    if (!g_headless) hwdisp_deinit();
    exit(code);
}

void sf3500_error_screen(const char* line1, const char* line2) {
    unsigned short* px;
    int i;
    if (!g_surf) return;
    px = (unsigned short*)g_surf->pixels;
    for (i = 0; i < SF_W * SF_H; i++) px[i] = 0;
    ttfshim_draw565(px, g_surf->pitch / 2, SF_W, SF_H, 24, 120, 2, 0xFFFF, "SkiFree");
    ttfshim_draw565(px, g_surf->pitch / 2, SF_W, SF_H, 24, 170, 2, 0xFFE0, line1);
    ttfshim_draw565(px, g_surf->pitch / 2, SF_W, SF_H, 24, 210, 2, 0xFFFF, line2);
    ttfshim_draw565(px, g_surf->pitch / 2, SF_W, SF_H, 24, 400, 2, 0x8410, "Press SELECT+START to exit");
    if (!g_headless) hwdisp_present(g_surf->pixels, SF_W, SF_H, g_surf->pitch);
    if (g_shots) { g_frame = 0; save_shot(); }
    if (g_headless) return;
    for (;;) {
        unsigned b = sf3500_buttons();
        if ((b & SB_START) && (b & SB_SELECT)) break;
        SDL_Delay(50);
    }
}
