#include "sf3500.h"
#include "SDL_ttf.h"
#include "hwdisp.hpp"
#include "joykey.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

static SDL_Surface* g_surf;
static SDL_Renderer* g_ren;
static int g_zoom = 1, g_fps = 40, g_headless, g_shot_every, g_maxframes;
static unsigned g_frame, g_last_ticks;
static const char* g_shots;
static char g_script[1024];

static int env_int(const char* name, int def) {
    const char* v = getenv(name);
    return (v && *v) ? atoi(v) : def;
}

int sf3500_start(void) {
    const char* s;
    g_zoom = env_int("SKI_ZOOM", 1);
    if (g_zoom < 1) g_zoom = 1;
    if (g_zoom > 3) g_zoom = 3;
    g_fps = env_int("SKI_FPS", 40);
    if (g_fps < 5) g_fps = 5;
    if (g_fps > 120) g_fps = 120;
    g_headless = env_int("SKI_HEADLESS", 0);
    g_shots = getenv("SKI_SHOTS");
    g_shot_every = env_int("SKI_SHOT_EVERY", 0);
    g_maxframes = env_int("SKI_MAXFRAMES", 0);
    s = getenv("SKI_JOY");
    if (s) snprintf(g_script, sizeof(g_script), "%s", s);

    /* the card's background scripts wake up often; a better priority keeps frames even
     * (Water Closet: late frames 14-19 per 2 s -> 0 with nice -10) */
    s = getenv("SKI_NICE");
    if (s && *s) setpriority(PRIO_PROCESS, 0, atoi(s));

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
    fprintf(stderr, "sf3500: zoom %d, fps cap %d%s\n", g_zoom, g_fps, g_headless ? ", headless" : "");
    return 0;
}

SDL_Renderer* sf3500_make_renderer(void) {
    g_ren = SDL_CreateSoftwareRenderer(g_surf);
    if (g_ren && g_zoom > 1) SDL_RenderSetLogicalSize(g_ren, SF_W / g_zoom, SF_H / g_zoom);
    return g_ren;
}

void sf3500_output_size(int* w, int* h) {
    *w = SF_W / g_zoom;
    *h = SF_H / g_zoom;
}

static void save_shot(void) {
    char path[512];
    snprintf(path, sizeof(path), "%s/frame%06u.bmp", g_shots, g_frame);
    SDL_SaveBMP(g_surf, path);
}

void sf3500_present(void) {
    unsigned now, target;
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
