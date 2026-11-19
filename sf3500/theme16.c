#include "theme16.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define LW 320
#define LH 240

static unsigned short L[LH][LW];

static unsigned short rgb(int r, int g, int b) {
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static unsigned rng_state = 20261118u;
static int rnd(int n) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return (int)((rng_state >> 8) % (unsigned)n);
}

static void P(int x, int y, unsigned short c) {
    if (x >= 0 && x < LW && y >= 0 && y < LH) L[y][x] = c;
}
static void R(int x, int y, int w, int h, unsigned short c) {
    int i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) P(x + i, y + j, c);
}
static void LINE(int x0, int y0, int x1, int y1, unsigned short c, int thick) {
    int dx = abs(x1 - x0), dy = -abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (;;) {
        R(x0, y0, thick, thick, c);
        if (x0 == x1 && y0 == y1) break;
        {
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
}
/* filled ellipse; calls shade(x, y, nx, ny) for each pixel, nx/ny in -1..1 */
typedef unsigned short (*ShadeFn)(int x, int y, double nx, double ny, void* u);
static void ELL(int cx, int cy, int rx, int ry, ShadeFn f, void* u) {
    int x, y;
    for (y = -ry; y <= ry; y++)
        for (x = -rx; x <= rx; x++) {
            double nx = (double)x / rx, ny = (double)y / ry;
            if (nx * nx + ny * ny <= 1.0) {
                unsigned short c = f(cx + x, cy + y, nx, ny, u);
                if (c != 0xFFFF || 1) P(cx + x, cy + y, c);
            }
        }
}

static const int bayer[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };

/* ---- sky and mountains ---- */
static int g_variant;   /* 0 = the 16-bit look, 1 = the 8-bit look: bold flat bands, no dithering */

static void sky(void) {
    int x, y;
    if (g_variant) {   /* flat sky bands, like an 8-bit title screen */
        static const unsigned short band[6] = { 0, 0, 0, 0, 0, 0 };
        unsigned short bc[6];
        (void)band;
        bc[0] = rgb(36, 96, 236); bc[1] = rgb(56, 124, 248); bc[2] = rgb(92, 160, 252); bc[3] = rgb(140, 196, 254); bc[4] = rgb(190, 224, 254); bc[5] = rgb(226, 238, 252);
        for (y = 0; y < LH; y++)
            for (x = 0; x < LW; x++) { int b = y * 6 / 124; if (b > 5) b = 5; L[y][x] = bc[b]; }
        return;
    }
    for (y = 0; y < LH; y++)
        for (x = 0; x < LW; x++) {
            double t = y / 120.0;
            int r, g, b, d;
            if (t > 1) t = 1;
            r = (int)(96 + (226 - 96) * t); g = (int)(150 + (238 - 150) * t); b = (int)(226 + (252 - 226) * t);
            d = bayer[y & 3][x & 3] - 8;                      /* ordered dither, 16-bit banding feel */
            P(x, y, rgb((r + d / 2) & ~7, (g + d / 2) & ~7, (b + d / 2) & ~7));
        }
    /* a few soft clouds: dithered white pills */
    {
        int i;
        for (i = 0; i < 6; i++) {
            int cx = 20 + rnd(280), cy = 10 + rnd(40), w = 18 + rnd(22);
            int k;
            for (k = 0; k < w; k++) {
                int h = (int)(3 * sin(3.14159 * k / w)) + 1;
                int j;
                for (j = -h; j <= 0; j++) if (((cx + k + cy + j) & 1) == 0 || j == -h + 1) P(cx + k, cy + j, rgb(236, 244, 255));
            }
        }
    }
}

static void mountain(int cx, int peak, int half, int base, unsigned short lit, unsigned short shade, unsigned short cap, int capdepth) {
    int y, x;
    for (y = peak; y <= base; y++) {
        double t = (double)(y - peak) / (base - peak);
        int hw = (int)(half * t);
        for (x = cx - hw; x <= cx + hw; x++) {
            int jag = ((x * 7 + y * 3) % 5) - 2;
            unsigned short c = (x < cx + (int)(3 * sin(y * 0.35))) ? lit : shade;
            if (y - peak < capdepth + jag + (x < cx ? 0 : 1)) c = (x < cx) ? cap : rgb(206, 220, 248);
            if (x == cx - hw || x == cx + hw) c = rgb(86, 110, 170);
            P(x, y, c);
        }
    }
}

/* ---- snow ground ---- */
static unsigned short drift_shade(int x, int y, double nx, double ny, void* u) {
    (void)u;
    if (ny > 0.55) return rgb(176, 198, 238) ;                       /* shadow underneath */
    if (ny < -0.62) return rgb(255, 255, 255);                       /* lit rim */
    if (((x + y) & 1) && ny > 0.3) return rgb(206, 220, 246);
    return rgb(244, 248, 255);
}
static void drift(int cx, int cy, int rx, int ry) { ELL(cx, cy, rx, ry, drift_shade, 0); }

static void ground(void) {
    int x, y, i;
    for (y = 104; y < LH; y++)
        for (x = 0; x < LW; x++) {
            double t = (y - 104) / 136.0;
            int d = bayer[y & 3][x & 3] - 8;
            int r = (int)(234 + 20 * t) + d / 3, g = (int)(242 + 12 * t) + d / 3, b = 252 + d / 4;
            P(x, y, rgb(r & ~3, g & ~3, b));
        }
    /* rolling horizon: blue shadow line with soft drifts */
    for (x = 0; x < LW; x++) {
        int h = 104 + (int)(4 * sin(x / 21.0) + 3 * sin(x / 9.0 + 1.0));
        P(x, h, rgb(160, 184, 232)); P(x, h + 1, rgb(196, 214, 244));
    }
    for (i = 0; i < 16; i++) {
        int cx = rnd(LW), cy = 120 + rnd(115), rx = 14 + rnd(26), ry = 3 + rnd(4);
        drift(cx, cy, rx, ry);
    }
    /* sparkles */
    for (i = 0; i < 60; i++) {
        int sx = rnd(LW), sy = 110 + rnd(130);
        P(sx, sy, rgb(255, 255, 255)); if (!(i & 3)) { P(sx - 1, sy, rgb(214, 228, 252)); P(sx + 1, sy, rgb(214, 228, 252)); }
    }
}

/* ---- pines with snow ---- */
static void pine(int cx, int base, int h) {
    int tiers = 4, i, y, x;
    unsigned short dark = rgb(18, 84, 52), mid = rgb(34, 122, 74), lite = rgb(72, 168, 98), line = rgb(10, 52, 36);
    /* shadow on the snow */
    for (x = -h / 3; x <= h / 3; x++) P(cx + x + h / 8, base + 1 + (x & 1), rgb(176, 198, 238));
    R(cx - 2, base - h / 7, 4, h / 7, rgb(112, 70, 36)); R(cx - 2, base - h / 7, 1, h / 7, rgb(150, 98, 52));
    for (i = 0; i < tiers; i++) {
        int top = base - h + (i * (h - h / 7)) / tiers - (i ? h / 14 : 0);
        int bot = base - h / 7 - ((tiers - 1 - i) * (h - h / 7)) / tiers + h / 12;
        int maxw = (h * (i + 2)) / (tiers * 3) + 2;
        if (bot > base - h / 7 + 2) bot = base - h / 7 + 2;
        for (y = top; y <= bot; y++) {
            double t = (double)(y - top) / (bot - top + 1);
            int hw = (int)(maxw * (0.25 + 0.75 * t));
            for (x = -hw; x <= hw; x++) {
                unsigned short c = x < -hw / 3 ? lite : (x < hw / 3 ? mid : dark);
                if (x == -hw || x == hw || y == bot) c = line;
                else if (y < top + 2 + (i == 0 ? 3 : 1) && x < hw / 2 && x > -hw) c = rgb(250, 252, 255);   /* snow cap */
                else if (t < 0.35 && x < 0 && ((x + y) & 1) == 0) c = rgb(228, 238, 252);                   /* snow patches */
                P(cx + x, y, c);
            }
        }
    }
}

/* ---- ski lift ---- */
static void lift(void) {
    unsigned short cable = rgb(40, 42, 54), dark = rgb(30, 32, 42);
    LINE(0, 10, 100, 100, cable, 2);
    /* tower */
    R(86, 96, 9, 3, dark);
    R(80, 94, 3, 6, dark); R(98, 94, 3, 6, dark);
    R(88, 99, 5, 44, dark); R(89, 99, 1, 44, rgb(80, 84, 100));
    R(88, 128, 5, 15, rgb(244, 204, 40)); R(88, 128, 5, 1, rgb(190, 150, 20));
    /* chair on a hanger */
    LINE(34, 45, 34, 56, cable, 1);
    R(24, 56, 22, 12, rgb(30, 16, 20));
    R(25, 57, 20, 10, rgb(214, 36, 36));
    R(25, 60, 20, 3, rgb(250, 244, 244));
    R(25, 57, 20, 1, rgb(244, 120, 110));
}

/* ---- yeti ---- */
static unsigned short yeti_fur(int x, int y, double nx, double ny, void* u) {
    (void)u;
    if (nx * nx + ny * ny > 0.82) return rgb(48, 48, 60);
    if (((x * 3 + y * 5) % 7) == 0) return rgb(176, 178, 188);
    if (nx < -0.2 && ny < 0.3) return rgb(150, 152, 162);
    return rgb(122, 124, 136);
}
static void yeti(int cx, int base) {
    unsigned short dk = rgb(46, 46, 58), fur = rgb(124, 126, 138), lt = rgb(178, 180, 190), claw = rgb(70, 72, 84);
    int i;
    /* arms: chain-like, raised */
    LINE(cx - 21, 62, cx - 44, 40, dk, 3); LINE(cx - 21, 62, cx - 44, 40, rgb(150, 152, 162), 1);
    LINE(cx + 21, 60, cx + 42, 22, dk, 3); LINE(cx + 21, 60, cx + 42, 22, rgb(150, 152, 162), 1);
    for (i = 0; i < 4; i++) {                                   /* claws */
        LINE(cx - 44, 40, cx - 52 + i * 3, 30 + (i & 1) * 2, claw, 1);
        LINE(cx + 42, 22, cx + 36 + i * 4, 10 + (i & 1) * 2, claw, 1);
    }
    /* body */
    ELL(cx, base - 36, 27, 35, yeti_fur, 0);
    R(cx - 17, 40, 34, 10, fur);
    /* head: block with horns */
    R(cx - 13, 24, 26, 24, dk); R(cx - 12, 25, 24, 22, fur);
    R(cx - 12, 25, 24, 3, lt);
    R(cx - 16, 20, 4, 8, dk); R(cx + 12, 20, 4, 8, dk); R(cx - 15, 18, 2, 3, dk); R(cx + 13, 18, 2, 3, dk);
    R(cx - 10, 28, 20, 18, rgb(150, 152, 162));
    /* brows and eyes */
    R(cx - 9, 29, 7, 2, dk); R(cx + 2, 29, 7, 2, dk);
    R(cx - 8, 32, 4, 3, rgb(232, 28, 28)); R(cx + 4, 32, 4, 3, rgb(232, 28, 28));
    R(cx - 7, 32, 1, 1, rgb(255, 160, 150)); R(cx + 5, 32, 1, 1, rgb(255, 160, 150));
    /* mouth with teeth */
    R(cx - 10, 38, 20, 7, dk);
    for (i = 0; i < 4; i++) R(cx - 8 + i * 5, 39, 3, 5, rgb(250, 216, 40));
}

/* ---- skier ---- */
static void skier(int cx, int fy) {
    unsigned short dk = rgb(40, 40, 50), jac = rgb(214, 54, 192), jacd = rgb(160, 30, 140), vest = rgb(40, 92, 224),
                   pant = rgb(38, 170, 52), boot = rgb(246, 210, 40), face = rgb(244, 206, 70), hat = rgb(222, 36, 40);
    /* trail behind */
    LINE(cx - 70, fy - 16, cx - 42, fy - 4, rgb(186, 204, 244), 1);
    LINE(cx - 70, fy - 20, cx - 36, fy - 8, rgb(206, 220, 248), 1);
    LINE(cx - 36, fy - 8, cx - 20, fy, rgb(186, 204, 244), 1);
    /* skis */
    LINE(cx - 20, fy - 2, cx + 18, fy + 6, dk, 2); LINE(cx - 18, fy - 4, cx + 16, fy + 3, rgb(150, 152, 164), 1);
    LINE(cx - 18, fy + 1, cx + 20, fy + 8, rgb(70, 72, 86), 1);
    /* boots and legs */
    R(cx - 9, fy - 8, 6, 6, boot); R(cx + 3, fy - 4, 6, 6, boot);
    R(cx - 9, fy - 20, 6, 12, pant); R(cx + 3, fy - 18, 6, 14, pant);
    R(cx - 9, fy - 20, 2, 12, rgb(90, 214, 100));
    /* torso */
    R(cx - 11, fy - 38, 22, 20, jac); R(cx - 11, fy - 38, 3, 20, rgb(240, 120, 220)); R(cx + 8, fy - 38, 3, 20, jacd);
    R(cx - 6, fy - 36, 12, 16, vest); R(cx - 6, fy - 36, 2, 16, rgb(100, 150, 250));
    R(cx - 8, fy - 22, 16, 2, rgb(120, 70, 40));
    /* arms and poles */
    LINE(cx - 11, fy - 34, cx - 17, fy - 22, jac, 3); R(cx - 20, fy - 24, 4, 4, boot);
    LINE(cx + 11, fy - 34, cx + 17, fy - 24, jac, 3); R(cx + 16, fy - 26, 4, 4, boot);
    LINE(cx - 18, fy - 22, cx - 24, fy - 2, dk, 1); LINE(cx + 18, fy - 24, cx + 24, fy - 2, dk, 1);
    /* head, goggles, hat */
    R(cx - 6, fy - 50, 12, 12, face);
    R(cx - 6, fy - 47, 12, 4, rgb(30, 64, 190)); R(cx - 5, fy - 47, 3, 1, rgb(160, 200, 255));
    R(cx - 3, fy - 41, 6, 1, rgb(150, 90, 30));
    R(cx - 7, fy - 56, 14, 7, hat); R(cx - 7, fy - 56, 14, 1, rgb(248, 110, 100));
    LINE(cx + 6, fy - 54, cx + 16, fy - 52, hat, 3); R(cx + 15, fy - 54, 4, 4, rgb(50, 110, 240));
    LINE(cx - 7, fy - 54, cx - 12, fy - 48, hat, 2);
}

void theme16_render(unsigned short* dst, int stride_px, int variant) {
    int x, y;
    g_variant = variant;
    rng_state = 20261118u;
    sky();
    if (variant) {   /* violet-blue ranges with flat faces */
        mountain(70, 52, 70, 112, rgb(120, 110, 220), rgb(84, 70, 190), rgb(252, 252, 254), 14);
        mountain(150, 40, 80, 112, rgb(104, 96, 210), rgb(68, 56, 176), rgb(252, 252, 254), 16);
        mountain(236, 56, 64, 112, rgb(124, 114, 224), rgb(88, 74, 194), rgb(252, 252, 254), 12);
        mountain(300, 48, 52, 112, rgb(110, 100, 214), rgb(74, 60, 182), rgb(252, 252, 254), 13);
    } else {
        mountain(70, 52, 70, 112, rgb(142, 166, 214), rgb(112, 138, 196), rgb(246, 250, 255), 14);
        mountain(150, 40, 80, 112, rgb(130, 156, 208), rgb(100, 126, 188), rgb(250, 252, 255), 16);
        mountain(236, 56, 64, 112, rgb(146, 170, 216), rgb(116, 142, 198), rgb(246, 250, 255), 12);
        mountain(300, 48, 52, 112, rgb(134, 160, 210), rgb(104, 130, 190), rgb(250, 252, 255), 13);
    }
    ground();
    lift();
    drift(90, 146, 12, 3);
    yeti(250, 108);
    drift(252, 111, 56, 15); drift(212, 109, 28, 9); drift(292, 110, 32, 10);
    pine(28, 150, 58); pine(70, 212, 70); pine(18, 226, 42);
    pine(300, 160, 54); pine(272, 218, 44);
    skier(150, 190);
    for (y = 0; y < LH; y++)
        for (x = 0; x < LW; x++) {
            unsigned short c = L[y][x];
            unsigned short* d = dst + (y * 2) * stride_px + x * 2;
            d[0] = d[1] = c; d[stride_px] = d[stride_px + 1] = c;
        }
}
