#include "launcher.h"
#include "config.h"
#include "sf3500.h"
#include "sfsound.h"
#include "SDL_ttf.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#define C(r, g, b) ((unsigned short)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
/* Windows 3.x look, to match the backdrop: gray bevelled dialogs, navy title bar, SkiFree yellow */
#define COL_BG      C(226, 236, 248)
#define COL_SNOW    C(250, 252, 255)
#define COL_TITLE   C(18, 38, 108)
#define COL_SHADOW  C(160, 178, 214)
#define COL_PINE    C(22, 94, 56)
#define COL_PINE2   C(34, 124, 70)
#define COL_TRUNK   C(104, 70, 40)
#define COL_GRAY    C(192, 192, 192)
#define COL_DGRAY   C(128, 128, 128)
#define COL_WHITE   C(255, 255, 255)
#define COL_BLACK   C(0, 0, 0)
#define COL_NAVY    C(0, 0, 168)
#define COL_YELLOW  C(250, 235, 20)
#define COL_DROP    C(70, 70, 96)
#define COL_HINT    C(0, 0, 120)

typedef struct { int v; const char* t; } Opt;

enum { K_CHOICE, K_ACTION };
typedef struct {
    int kind;
    const char* name;
    int* val;            /* K_CHOICE: the config value */
    const Opt* opts;
    int n;
    int id;              /* K_ACTION */
    const char* help;
} Item;

enum { A_PLAY = 1, A_AV, A_GAME, A_EXIT, A_BACK, A_RESET_AV, A_RESET_GAME, A_HOW };

static const Opt o_onoff[] = { { 1, "On" }, { 0, "Off" } };
static const Opt o_volume[] = { { 0, "Mute" }, { 1, "1 of 10" }, { 2, "2 of 10" }, { 3, "3 of 10" }, { 4, "4 of 10" },
                                { 5, "5 of 10" }, { 6, "6 of 10" }, { 7, "7 of 10" }, { 8, "8 of 10" },
                                { 9, "9 of 10" }, { 10, "10 of 10" } };
static const Opt o_set[] = { { 1, "Custom" }, { 0, "Built-in" } };
static const Opt o_screen[] = { { 0, "640x480 full" }, { 1, "480x360 window" }, { 2, "320x240 window" },
                                { 3, "2x full screen" }, { 4, "2x window" } };
static const Opt o_fps[] = { { 20, "20 fps" }, { 30, "30 fps" }, { 40, "40 fps" }, { 60, "60 fps" } };
static const Opt o_timing[] = { { 0, "Smooth" }, { 1, "Original" } };
static const Opt o_yeti[] = { { 0, "Never" }, { 250, "250 m" }, { 500, "500 m" },
                              { 1000, "1000 m" }, { 2000, "2000 m (orig)" } };
static const Opt o_speed[] = { { 50, "50%" }, { 75, "75%" }, { 100, "100% (orig)" }, { 125, "125%" }, { 150, "150%" } };

#define N(a) ((int)(sizeof(a) / sizeof((a)[0])))

static char g_help_sound[200];
static char g_help_screen[200];

static const Item main_items[] = {
    { K_ACTION, "Play Game", 0, 0, 0, A_PLAY, "Start skiing with the settings below." },
    { K_ACTION, "How To Play", 0, 0, 0, A_HOW, "How SkiFree works and what the buttons do." },
    { K_ACTION, "Audio / Video", 0, 0, 0, A_AV, "Sounds, volume, screen size and frame rate." },
    { K_ACTION, "Gameplay", 0, 0, 0, A_GAME, "Frame timing, yeti and game speed." },
    { K_ACTION, "Exit", 0, 0, 0, A_EXIT, "Leave SkiFree and go back to the menu." },
};

static Item av_items[] = {
    { K_CHOICE, "Sound", &g_cfg.sound, o_onoff, N(o_onoff), 0, "Off plays nothing, even if custom sounds are installed." },
    { K_CHOICE, "Volume", &g_cfg.volume, o_volume, N(o_volume), 0, "Mute silences the game but keeps your setting." },
    { K_CHOICE, "Sound set", &g_cfg.custom, o_set, N(o_set), 0, g_help_sound },
    { K_CHOICE, "Screen", &g_cfg.mode, o_screen, N(o_screen), 0, g_help_screen },
    { K_CHOICE, "Frame rate", &g_cfg.fps, o_fps, N(o_fps), 0, "Frame rate cap. The game keeps its speed at any rate." },
    { K_ACTION, "Restore defaults", 0, 0, 0, A_RESET_AV, "Put the audio and video settings back." },
};

static Item game_items[] = {
    { K_CHOICE, "Frame timing", &g_cfg.timing, o_timing, N(o_timing), 0,
      "Smooth moves the game every frame. Original steps it every 40 ms like Windows, a bit choppier." },
    { K_CHOICE, "Yeti", &g_cfg.yeti, o_yeti, N(o_yeti), 0,
      "How far you ski before the yeti shows up. Never = practice mode, no yeti at all." },
    { K_CHOICE, "Game speed", &g_cfg.speed, o_speed, N(o_speed), 0,
      "Slow the whole game down to practise, or speed it up." },
    { K_ACTION, "Restore defaults", 0, 0, 0, A_RESET_GAME, "Put the gameplay settings back." },
};

static int opt_index(const Item* it) {
    int i;
    for (i = 0; i < it->n; i++)
        if (it->opts[i].v == *it->val) return i;
    return 0;
}

/* ---- drawing ---- */

static unsigned short* g_px;
static int g_stride;
static SDL_Surface* g_bg;     /* launcher.bmp, 640x480 RGB565; NULL = drawn fallback */

static void load_bg(void) {
    SDL_Surface* raw = SDL_LoadBMP("launcher.bmp");
    if (!raw) return;
    if (raw->w == SF_W && raw->h == SF_H) g_bg = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGB565, 0);
    SDL_FreeSurface(raw);
}

static void fill(int x, int y, int w, int h, unsigned short c) {
    int i, j;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SF_W) w = SF_W - x;
    if (y + h > SF_H) h = SF_H - y;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) g_px[(y + j) * g_stride + x + i] = c;
}

static void text(int x, int y, int scale, unsigned short c, const char* s) {
    ttfshim_draw565(g_px, g_stride, SF_W, SF_H, x, y, scale, c, s);
}

static int twidth(int scale, const char* s) { return (int)strlen(s) * 8 * scale; }

/* Windows 3.x bevel: raised = white top-left, dark bottom-right; sunken the other way round */
static void box3d(int x, int y, int w, int h, int sunken, unsigned short face) {
    unsigned short hi = sunken ? COL_DGRAY : COL_WHITE, lo = sunken ? COL_WHITE : COL_DGRAY;
    fill(x, y, w, h, COL_BLACK);
    fill(x + 1, y + 1, w - 2, h - 2, face);
    fill(x + 1, y + 1, w - 2, 2, hi);
    fill(x + 1, y + 1, 2, h - 2, hi);
    fill(x + 1, y + h - 3, w - 2, 2, lo);
    fill(x + w - 3, y + 1, 2, h - 2, lo);
}

static void arrow(int x, int cy, int dir, unsigned short c) {   /* 6 wide triangle centered on cy */
    int i;
    for (i = 0; i < 6; i++) {
        int half = dir < 0 ? i + 1 : 6 - i;
        fill(x + i, cy - half, 1, half * 2, c);
    }
}

static void tree(int cx, int base, int h) {
    int i, tw = h / 6 > 3 ? h / 6 : 3;
    fill(cx - tw / 2, base - h / 6, tw, h / 6, COL_TRUNK);
    for (i = 0; i < 3; i++) {
        int top = base - h + i * (h / 4);
        int hh = h / 2 + h / 8;
        int row;
        for (row = 0; row < hh; row++) {
            int half = (row * (h / 4 + i * h / 14)) / hh + 1;
            fill(cx - half, top + row, half * 2, 1, (row & 4) ? COL_PINE2 : COL_PINE);
        }
    }
}

static void background(void) {
    int y, x;
    if (g_bg) {
        for (y = 0; y < SF_H; y++)
            memcpy(g_px + y * g_stride, (unsigned char*)g_bg->pixels + y * g_bg->pitch, SF_W * 2);
        return;
    }
    /* no launcher.bmp: plain snowy fallback */
    fill(0, 0, SF_W, SF_H, COL_BG);
    for (x = 0; x < SF_W; x++) {
        int top = 400 + (int)(9.0 * sin(x / 53.0) + 5.0 * sin(x / 19.0 + 1.0));
        fill(x, top, 1, SF_H - top, COL_SNOW);
    }
    tree(34, 150, 60); tree(606, 150, 60); tree(24, 470, 56); tree(616, 470, 56);
    text(SF_W / 2 - twidth(4, "SkiFree") / 2 + 3, 25, 4, COL_SHADOW, "SkiFree");
    text(SF_W / 2 - twidth(4, "SkiFree") / 2, 22, 4, COL_TITLE, "SkiFree");
}

/* big raised button; the selected one is navy with yellow text */
static void button(int x, int y, int w, int h, const char* label, int selected) {
    int tx = x + (w - twidth(2, label)) / 2, ty = y + (h - 16) / 2;
    box3d(x, y, w, h, 0, selected ? COL_NAVY : COL_GRAY);
    if (selected) {
        fill(x + 4, y + 4, w - 8, 1, COL_WHITE); fill(x + 4, y + h - 5, w - 8, 1, COL_WHITE);
        fill(x + 4, y + 4, 1, h - 8, COL_WHITE); fill(x + w - 5, y + 4, 1, h - 8, COL_WHITE);
    }
    text(tx, ty, 2, selected ? COL_YELLOW : COL_BLACK, label);
}

static int wrap_text(int x, int y, int cols, int maxlines, unsigned short c, const char* s) {
    char line[80];
    int lines = 0;
    if (cols > 78) cols = 78;
    while (*s && lines < maxlines) {
        int n = (int)strlen(s), cut;
        if (n > cols) {
            cut = cols;
            while (cut > 0 && s[cut] != ' ') cut--;
            if (cut == 0) cut = cols;
        } else cut = n;
        memcpy(line, s, (size_t)cut);
        line[cut] = 0;
        text(x, y + lines * 11, 1, c, line);
        s += cut;
        while (*s == ' ') s++;
        lines++;
    }
    return lines;
}

/* tiny picture of the screen with the game's window in it */
static void preview(int x, int y, int mode) {
    int vw, vh, sc, ww, wh, pw = 44, ph = 33;
    sf3500_mode_info(mode, &vw, &vh, &sc, &ww, &wh);
    fill(x - 1, y - 1, pw + 2, ph + 2, COL_BLACK);
    fill(x, y, pw, ph, COL_BLACK);
    fill(x + (pw - ww * pw / SF_W) / 2, y + (ph - wh * ph / SF_H) / 2, ww * pw / SF_W, wh * ph / SF_H, COL_WHITE);
}

static void hint(void) {
    const char* h = "UP/DOWN choose  LEFT/RIGHT change  A select  B back  L1/R1 theme";
    text(SF_W / 2 + 8 - twidth(1, h) / 2, 452, 1, COL_HINT, h);
}

/* ---- TI-92 theme: the calculator's LCD, four grays, a tab toolbar on top, a status line below ---- */

#define TI0 C(190, 204, 180)
#define TI1 C(146, 164, 140)
#define TI2 C(84, 102, 92)
#define TI3 C(26, 36, 38)

static const char* const k_tabs[5] = { "Play", "Help", "A / V", "Game", "Exit" };

static void ti_outline(int x, int y, int w, int h, unsigned short c) {
    fill(x, y, w, 2, c); fill(x, y + h - 2, w, 2, c);
    fill(x, y, 2, h, c); fill(x + w - 2, y, 2, h, c);
}

/* active: the tab the body belongs to (-1 none); cursor: tab shown inverted (-1 none) */
static void ti_frame(int active, int cursor) {
    int i;
    fill(0, 0, SF_W, SF_H, TI3);                 /* bezel */
    fill(8, 8, SF_W - 16, SF_H - 16, TI0);       /* LCD */
    ti_outline(14, 44, SF_W - 28, 372, TI3);     /* body */
    for (i = 0; i < 5; i++) {
        int x = 14 + i * 124, y = 12, w = 116, h = 34;
        unsigned short face = (i == cursor) ? TI3 : (i == active ? TI0 : TI1);
        fill(x, y, w, h, face);
        fill(x, y, w, 2, TI3); fill(x, y, 2, h, TI3); fill(x + w - 2, y, 2, h, TI3);
        fill(x, y, 4, 4, TI0); fill(x + w - 4, y, 4, 4, TI0);          /* cut corners */
        fill(x + 2, y + 2, 2, 2, TI3); fill(x + w - 4, y + 2, 2, 2, TI3);
        if (i == active) fill(x + 2, 44, w - 4, 2, TI0);                /* tab opens into the body */
        text(x + (w - twidth(2, k_tabs[i])) / 2, y + 11, 2, i == cursor ? TI0 : TI3, k_tabs[i]);
    }
    ti_outline(14, 424, 476, 44, TI3);           /* status line: help */
    ti_outline(494, 424, SF_W - 14 - 494, 44, TI3);
    text(502, 431, 1, TI3, "THEME");
    text(502, 442, 1, TI3, "TI-92");
    text(502, 453, 1, TI2, "L1/R1");
}

static void ti_hint(void) {
    const char* h = "UP/DOWN choose  LEFT/RIGHT change  A select  B back";
    text(SF_W / 2 - twidth(1, h) / 2, 398, 1, TI2, h);
}

static void draw_main_ti(int sel, const Item* items, int n) {
    int i;
    const char* t = "SkiFree";
    ti_frame(-1, sel);
    text(SF_W / 2 - twidth(6, t) / 2 + 3, 63, 6, TI1, t);
    text(SF_W / 2 - twidth(6, t) / 2, 60, 6, TI3, t);
    fill(60, 118, SF_W - 120, 2, TI3);
    text(SF_W / 2 - twidth(1, "by Chris Pirih") / 2, 126, 1, TI2, "by Chris Pirih");
    for (i = 0; i < n; i++) {
        int y = 148 + i * 34;
        if (i == sel) fill(150, y - 4, SF_W - 300, 30, TI3);
        text(SF_W / 2 - twidth(2, items[i].name) / 2, y + 2, 2, i == sel ? TI0 : TI3, items[i].name);
    }
    ti_hint();
    wrap_text(22, 431, 57, 3, TI3, items[sel].help);
}

static void draw_sub_ti(const char* title, const Item* items, int n, int sel) {
    int i, y = 60, pv;
    char buf[48];
    ti_frame(!strcmp(title, "Gameplay") ? 3 : 2, -1);
    for (i = 0; i < n; i++, y += 32) {
        const Item* it = &items[i];
        int selr = (i == sel);
        unsigned short fg = selr ? TI0 : TI3;
        if (selr) fill(24, y - 3, SF_W - 48, 30, TI3);
        if (it->kind == K_ACTION) {
            snprintf(buf, sizeof buf, "[ %s ]", it->name);
            text(SF_W / 2 - twidth(2, buf) / 2, y + 3, 2, fg, buf);
            continue;
        }
        text(36, y + 3, 2, fg, it->name);
        {
            const char* v = it->opts[opt_index(it)].t;
            int vw = twidth(2, v), vx = SF_W - 36 - 24 - vw;
            text(vx, y + 3, 2, fg, v);
            if (selr) {
                arrow(vx - 16, y + 12, -1, TI0);
                arrow(SF_W - 36 - 12, y + 12, 1, TI0);
            }
        }
    }
    ti_hint();
    pv = items[sel].kind == K_CHOICE && items[sel].val == &g_cfg.mode;
    wrap_text(22, 431, pv ? 50 : 57, 3, TI3, items[sel].help);
    if (pv) preview(436, 428, g_cfg.mode);
}

#define PANEL_X 125
#define PANEL_W 430
#define PANEL_Y 138
#define ROW_H 24

static void draw_main(int sel, const Item* items, int n) {
    int i;
    if (g_cfg.look) { draw_main_ti(sel, items, n); return; }
    background();
    for (i = 0; i < n; i++) button(220, 144 + i * 48, 280, 40, items[i].name, i == sel);
    hint();
}

static void draw_sub(const char* title, const Item* items, int n, int sel) {
    int i, h = 81 + ROW_H * n, x = PANEL_X, y = PANEL_Y, w = PANEL_W, ry, sy;
    char buf[48];
    if (g_cfg.look) { draw_sub_ti(title, items, n, sel); return; }
    background();
    fill(x + 5, y + 5, w, h, COL_DROP);
    box3d(x, y, w, h, 0, COL_GRAY);
    fill(x + 3, y + 3, w - 6, 22, COL_NAVY);
    text(x + 10, y + 6, 2, COL_WHITE, title);
    ry = y + 33;
    for (i = 0; i < n; i++, ry += ROW_H) {
        const Item* it = &items[i];
        int selr = (i == sel);
        if (it->kind == K_ACTION) {
            button(x + 8 + (w - 16 - 290) / 2, ry, 290, ROW_H - 2, it->name, selr);
            continue;
        }
        if (selr) fill(x + 8, ry, w - 16, ROW_H - 2, COL_NAVY);
        text(x + 14, ry + 3, 2, selr ? COL_WHITE : COL_BLACK, it->name);
        {
            const char* v = it->opts[opt_index(it)].t;
            int fw = twidth(2, v) + 40, fx = x + w - 12 - fw;
            snprintf(buf, sizeof buf, "%s", v);
            box3d(fx, ry, fw, ROW_H - 2, 1, COL_WHITE);
            arrow(fx + 7, ry + (ROW_H - 2) / 2, -1, selr ? COL_NAVY : C(170, 170, 170));
            arrow(fx + fw - 13, ry + (ROW_H - 2) / 2, 1, selr ? COL_NAVY : C(170, 170, 170));
            text(fx + 20, ry + 3, 2, COL_BLACK, buf);
        }
    }
    /* status bar with the help line (and the screen preview on the Screen row) */
    sy = y + h - 8 - 36;
    box3d(x + 8, sy, w - 16, 36, 1, COL_GRAY);
    {
        int pv = items[sel].kind == K_CHOICE && items[sel].val == &g_cfg.mode;
        wrap_text(x + 15, sy + 5, (w - 16 - 14 - (pv ? 56 : 0)) / 8, 3, COL_BLACK, items[sel].help);
        if (pv) preview(x + w - 8 - 52, sy + 2, g_cfg.mode);
    }
    hint();
}

/* ---- input ---- */

static unsigned g_prev;
static unsigned g_next[4];

/* edge presses plus held-key repeat for the four directions */
static unsigned poll_pressed(void) {
    unsigned b = sf3500_buttons(), pressed = b & ~g_prev, held = b & g_prev;
    unsigned now = SDL_GetTicks();
    int i;
    for (i = 0; i < 4; i++) {
        unsigned m = 1u << i;
        if (pressed & m) g_next[i] = now + 350;
        else if ((held & m) && now >= g_next[i]) { pressed |= m; g_next[i] = now + 110; }
    }
    g_prev = b;
    if (pressed & (SB_L1 | SB_R1)) g_cfg.look = !g_cfg.look;     /* theme: only here, never in the game */
    if ((b & SB_START) && (b & SB_SELECT)) return 0x80000000u;   /* leave */
    return pressed;
}

/* ---- how to play ---- */

#define HOW_PAGES 2
#define HOW_X 116
#define HOW_W 446
#define HOW_Y 134
#define HOW_H 228

static const char* const k_controls[][2] = {
    { "LEFT/RIGHT", "turn" },
    { "DOWN", "straight downhill" },
    { "UP", "shuffle uphill when stopped" },
    { "Y / A", "steer down-left / down-right" },
    { "B or R1", "jump; again in the air = tricks" },
    { "X", "small hop" },
    { "SELECT", "pause" },
    { "START", "new game" },
    { "SELECT+START", "back to this menu" },
};

/* the text of a page, drawn from (x, ty), w wide, in the given colours */
static void howto_body(int x, int ty, int w, int page, unsigned short ct, unsigned short ck, unsigned short cd) {
    int i, cols = (w - 8) / 8;
    if (page == 0) {
        ty += 12 * wrap_text(x, ty, cols, 3, ct, "Ski down the mountain and stay on your feet. Trees and rocks knock you over.") + 4;
        ty += 12 * wrap_text(x, ty, cols, 2, ct, "Jump off bumps and ramps, and do tricks in the air for style points.") + 6;
        text(x, ty, 1, ck, "Pick a course by skiing through a sign at the start:");
        ty += 14;
        ty += 12 * wrap_text(x + 8, ty, cols - 2, 2, ct, "Slalom: a timed race through the gates. A missed gate adds 5 seconds.");
        ty += 12 * wrap_text(x + 8, ty, cols - 2, 2, ct, "Tree Slalom: a longer timed race.");
        ty += 12 * wrap_text(x + 8, ty, cols - 2, 2, ct, "Freestyle: no finish line. Score style points with jumps and tricks.") + 6;
        wrap_text(x, ty, cols, 3, ct, "After 2000 m the yeti comes after you. If he catches you, press B to start again.");
    } else {
        for (i = 0; i < (int)(sizeof k_controls / sizeof k_controls[0]); i++, ty += 15) {
            text(x, ty, 1, ck, k_controls[i][0]);
            text(x + 104, ty, 1, ct, k_controls[i][1]);
        }
        wrap_text(x, ty + 3, cols, 2, cd, "You get back up on your own after a fall.");
    }
}

static void draw_howto(int page) {
    int x = HOW_X, y = HOW_Y, w = HOW_W;
    char pg[16];
    snprintf(pg, sizeof pg, "%d/%d", page + 1, HOW_PAGES);
    if (g_cfg.look) {
        const char* h = "LEFT/RIGHT page  B back";
        ti_frame(1, -1);
        howto_body(36, 62, SF_W - 72, page, TI3, TI3, TI2);
        text(SF_W / 2 - twidth(1, h) / 2, 398, 1, TI2, h);
        text(SF_W - 36 - twidth(1, pg), 398, 1, TI3, pg);
        return;
    }
    background();
    fill(x + 5, y + 5, w, HOW_H, COL_DROP);
    box3d(x, y, w, HOW_H, 0, COL_GRAY);
    fill(x + 3, y + 3, w - 6, 22, COL_NAVY);
    text(x + 10, y + 6, 2, COL_WHITE, "How To Play");
    text(x + w - 10 - twidth(2, pg), y + 6, 2, COL_YELLOW, pg);
    box3d(x + 8, y + 31, w - 16, HOW_H - 31 - 8, 1, COL_WHITE);
    howto_body(x + 16, y + 38, w - 32, page, COL_BLACK, COL_NAVY, COL_DGRAY);
    {
        const char* h = "LEFT/RIGHT page    B back    L1/R1 theme";
        text(SF_W / 2 + 8 - twidth(1, h) / 2, 452, 1, COL_HINT, h);
    }
}

/* Returns A_BACK, or A_EXIT for SELECT+START. */
static int show_howto(void) {
    int page = 0, dirty = 1;
    for (;;) {
        unsigned p;
        if (dirty) { draw_howto(page); dirty = 0; }
        sf3500_present();
        p = poll_pressed();
        if (p) dirty = 1;
        if (p & 0x80000000u) return A_EXIT;
        if (p & (SB_RIGHT | SB_A)) page = (page + 1) % HOW_PAGES;
        if (p & SB_LEFT) page = (page + HOW_PAGES - 1) % HOW_PAGES;
        if (p & SB_B) return A_BACK;
    }
}

static void restore(int which) {
    SkiCfg d;
    SkiCfg keep = g_cfg;
    cfg_defaults();
    d = g_cfg;
    g_cfg = keep;
    if (which == A_RESET_AV) {
        g_cfg.sound = d.sound; g_cfg.volume = d.volume; g_cfg.custom = d.custom;
        g_cfg.mode = d.mode; g_cfg.fps = d.fps;
    } else {
        g_cfg.timing = d.timing; g_cfg.yeti = d.yeti; g_cfg.speed = d.speed;
    }
}

static void refresh_help(void) {
    int n = sfsnd_custom_count();
    int vw, vh, sc, ww, wh;
    char zs[16];
    if (n > 0)
        snprintf(g_help_sound, sizeof g_help_sound, "%d custom sound%s found in sounds/custom. Built-in ignores them.", n, n == 1 ? "" : "s");
    else
        snprintf(g_help_sound, sizeof g_help_sound, "No custom sounds found. Put .wav files in sounds/custom. Custom uses them, Built-in ignores them.");
    sf3500_mode_info(g_cfg.mode, &vw, &vh, &sc, &ww, &wh);
    if (sc > 1) snprintf(zs, sizeof zs, ", %dx zoom", sc); else zs[0] = 0;
    if (g_cfg.mode == 0)
        snprintf(g_help_screen, sizeof g_help_screen, "You see 640x480 of slope. Fills the screen.");
    else if (ww == SF_W && wh == SF_H)
        snprintf(g_help_screen, sizeof g_help_screen, "Everything %dx bigger: you see %dx%d of slope.", sc, vw, vh);
    else
        snprintf(g_help_screen, sizeof g_help_screen, "%dx%d window%s, you see %dx%d of slope.", ww, wh,
                 zs, vw, vh);
}

/* One menu screen. Returns the action that ended it (A_BACK, A_PLAY, A_EXIT, ...). */
static int run_menu(const char* subtitle, Item* items, int n, int is_main, int* selp) {
    int sel = *selp, dirty = 1;
    for (;;) {
        unsigned p;
        const Item* it;
        if (dirty) {
            refresh_help();
            if (is_main) draw_main(sel, items, n); else draw_sub(subtitle, items, n, sel);
            dirty = 0;
        }
        sf3500_present();
        p = poll_pressed();
        if (p) dirty = 1;
        *selp = sel;
        if (p & 0x80000000u) return A_EXIT;
        it = &items[sel];
        if (p & SB_UP) sel = (sel + n - 1) % n;
        if (p & SB_DOWN) sel = (sel + 1) % n;
        *selp = sel;
        it = &items[sel];
        if (it->kind == K_CHOICE && (p & (SB_LEFT | SB_RIGHT))) {
            int i = opt_index(it) + ((p & SB_RIGHT) ? 1 : it->n - 1);
            *it->val = it->opts[i % it->n].v;
            /* let the player hear the new level / set */
            if (it->val == &g_cfg.volume || it->val == &g_cfg.custom || it->val == &g_cfg.sound)
                sfsnd_play_by_name(it->val == &g_cfg.custom ? "gobble" : "crash");
        }
        if (it->kind == K_ACTION && ((p & SB_A) || ((p & SB_START) && it->id == A_PLAY))) {
            if (it->id == A_RESET_AV || it->id == A_RESET_GAME) restore(it->id);
            else return it->id;
        }
        if (it->kind == K_CHOICE && (p & SB_A)) {   /* A also steps the value */
            int i = opt_index(it) + 1;
            *it->val = it->opts[i % it->n].v;
        }
        if (p & SB_B) return A_BACK;
    }
}

int launcher_run(const char* cfg_path) {
    SkiCfg start = g_cfg;
    int result = 0;
    g_px = sf3500_pixels(&g_stride);
    load_bg();
    g_prev = 0xFFFFFFFFu;   /* ignore buttons already held when the menu opens */
    int msel = 0;
    for (;;) {
        int sub = 0, a = run_menu("", (Item*)main_items, N(main_items), 1, &msel);
        if (a == A_PLAY) { result = 1; break; }
        if (a == A_EXIT) { result = 0; break; }
        if (a == A_HOW) { if (show_howto() == A_EXIT) { result = 0; goto done; } }
        if (a == A_AV) { if (run_menu("Audio / Video", av_items, N(av_items), 0, &sub) == A_EXIT) { result = 0; goto done; } }
        if (a == A_GAME) { if (run_menu("Gameplay", game_items, N(game_items), 0, &sub) == A_EXIT) { result = 0; goto done; } }
    }
done:
    if (memcmp(&start, &g_cfg, sizeof start) != 0) {
        if (cfg_save(cfg_path) != 0) fprintf(stderr, "launcher: could not save %s\n", cfg_path);
    }
    return result;
}
