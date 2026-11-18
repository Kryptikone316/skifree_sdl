#include "launcher.h"
#include "config.h"
#include "sf3500.h"
#include "sfsound.h"
#include "SDL_ttf.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#define C(r, g, b) ((unsigned short)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define COL_BG      C(226, 236, 248)
#define COL_SNOW    C(250, 252, 255)
#define COL_TITLE   C(18, 38, 108)
#define COL_SHADOW  C(160, 178, 214)
#define COL_TEXT    C(26, 46, 96)
#define COL_SEL     C(32, 84, 176)
#define COL_SELTXT  C(255, 255, 255)
#define COL_DIM     C(96, 110, 146)
#define COL_PINE    C(22, 94, 56)
#define COL_PINE2   C(34, 124, 70)
#define COL_TRUNK   C(104, 70, 40)

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

enum { A_PLAY = 1, A_AV, A_GAME, A_EXIT, A_BACK, A_RESET_AV, A_RESET_GAME };

static const Opt o_onoff[] = { { 1, "On" }, { 0, "Off" } };
static const Opt o_volume[] = { { 0, "Mute" }, { 1, "1 of 10" }, { 2, "2 of 10" }, { 3, "3 of 10" }, { 4, "4 of 10" },
                                { 5, "5 of 10" }, { 6, "6 of 10" }, { 7, "7 of 10" }, { 8, "8 of 10" },
                                { 9, "9 of 10" }, { 10, "10 of 10" } };
static const Opt o_set[] = { { 1, "Custom if found" }, { 0, "Built-in only" } };
static const Opt o_screen[] = { { 0, "640x480 full" }, { 1, "480x360 window" }, { 2, "320x240 window" },
                                { 3, "2x full screen" }, { 4, "2x window" } };
static const Opt o_fps[] = { { 20, "20 fps" }, { 30, "30 fps" }, { 40, "40 fps" }, { 60, "60 fps" } };
static const Opt o_timing[] = { { 0, "Smooth" }, { 1, "Original" } };
static const Opt o_yeti[] = { { 0, "Never" }, { 250, "After 250 m" }, { 500, "After 500 m" },
                              { 1000, "After 1000 m" }, { 2000, "After 2000 m (orig)" } };
static const Opt o_speed[] = { { 50, "50%" }, { 75, "75%" }, { 100, "100% (original)" }, { 125, "125%" }, { 150, "150%" } };

#define N(a) ((int)(sizeof(a) / sizeof((a)[0])))

static char g_help_sound[200];
static char g_help_screen[200];

static const Item main_items[] = {
    { K_ACTION, "Play Game", 0, 0, 0, A_PLAY, "Start skiing with the settings below." },
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
    { K_ACTION, "Back", 0, 0, 0, A_BACK, "" },
};

static Item game_items[] = {
    { K_CHOICE, "Frame timing", &g_cfg.timing, o_timing, N(o_timing), 0,
      "Smooth moves the game every frame. Original steps it every 40 ms like Windows, a bit choppier." },
    { K_CHOICE, "Yeti", &g_cfg.yeti, o_yeti, N(o_yeti), 0,
      "How far you ski before the yeti shows up. Never = practice mode." },
    { K_CHOICE, "Game speed", &g_cfg.speed, o_speed, N(o_speed), 0,
      "Slow the whole game down to practise, or speed it up." },
    { K_ACTION, "Restore defaults", 0, 0, 0, A_RESET_GAME, "Put the gameplay settings back." },
    { K_ACTION, "Back", 0, 0, 0, A_BACK, "" },
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

static void background(const char* subtitle) {
    int x;
    fill(0, 0, SF_W, SF_H, COL_BG);
    /* a snowy drift across the bottom, rolling edge */
    for (x = 0; x < SF_W; x++) {
        int top = 384 + (int)(9.0 * sin(x / 53.0) + 5.0 * sin(x / 19.0 + 1.0));
        fill(x, top, 1, SF_H - top, COL_SNOW);
    }
    tree(34, 150, 60); tree(606, 150, 60);
    tree(24, 470, 56); tree(616, 470, 56);
    text(SF_W / 2 - twidth(4, "SkiFree") / 2 + 3, 25, 4, COL_SHADOW, "SkiFree");
    text(SF_W / 2 - twidth(4, "SkiFree") / 2, 22, 4, COL_TITLE, "SkiFree");
    text(SF_W / 2 - twidth(2, subtitle) / 2, 82, 2, COL_DIM, subtitle);
}

static void wrap_text(int x, int y, int cols, int scale, unsigned short c, const char* s) {
    char line[64];
    int lines = 0;
    while (*s && lines < 3) {
        int n = (int)strlen(s), cut;
        if (n > cols) {
            cut = cols;
            while (cut > 0 && s[cut] != ' ') cut--;
            if (cut == 0) cut = cols;
        } else cut = n;
        memcpy(line, s, (size_t)cut);
        line[cut] = 0;
        text(x, y + lines * 20 * scale / 2 * 1, scale, c, line);
        s += cut;
        while (*s == ' ') s++;
        lines++;
    }
}

/* little picture of the screen with the game's window in it */
static void preview(int x, int y, int mode) {
    int vw, vh, sc, ww, wh;
    sf3500_mode_info(mode, &vw, &vh, &sc, &ww, &wh);
    fill(x - 3, y - 3, 102, 78, COL_TITLE);
    fill(x, y, 96, 72, 0);
    fill(x + (96 - ww * 96 / SF_W) / 2, y + (72 - wh * 72 / SF_H) / 2, ww * 96 / SF_W, wh * 72 / SF_H, COL_SNOW);
}

static void draw_menu(const char* subtitle, const Item* items, int n, int sel, int mode_preview) {
    int i, y0 = 124;
    char buf[48];
    background(subtitle);
    for (i = 0; i < n; i++) {
        const Item* it = &items[i];
        int y = y0 + i * 36;
        unsigned short fg = COL_TEXT;
        if (i == sel) { fill(60, y - 6, 520, 32, COL_SEL); fg = COL_SELTXT; }
        text(80, y, 2, fg, it->name);
        if (it->kind == K_CHOICE) {
            const char* v = it->opts[opt_index(it)].t;
            if (i == sel) snprintf(buf, sizeof buf, "< %s >", v); else snprintf(buf, sizeof buf, "%s", v);
            text(560 - twidth(2, buf), y, 2, fg, buf);
        }
    }
    if (items[sel].help[0]) {
        int pv = mode_preview && items[sel].val == &g_cfg.mode;
        wrap_text(60, 398, pv ? 26 : 33, 2, COL_TEXT, items[sel].help);
        if (pv) preview(488, 387, g_cfg.mode);
    }
    text(SF_W / 2 - twidth(1, "UP/DOWN choose   LEFT/RIGHT change   A select   B back") / 2, 468, 1, COL_DIM,
         "UP/DOWN choose   LEFT/RIGHT change   A select   B back");
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
    if ((b & SB_START) && (b & SB_SELECT)) return 0x80000000u;   /* leave */
    return pressed;
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
        snprintf(g_help_sound, sizeof g_help_sound, "%d custom sound%s found in sounds/custom.", n, n == 1 ? "" : "s");
    else
        snprintf(g_help_sound, sizeof g_help_sound, "No custom sounds found. Put .wav files in sounds/custom.");
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
static int run_menu(const char* subtitle, Item* items, int n, int mode_preview, int* selp) {
    int sel = *selp, dirty = 1;
    for (;;) {
        unsigned p;
        const Item* it;
        if (dirty) {
            refresh_help();
            draw_menu(subtitle, items, n, sel, mode_preview);
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
    g_prev = 0xFFFFFFFFu;   /* ignore buttons already held when the menu opens */
    int msel = 0;
    for (;;) {
        int sub = 0, a = run_menu("Choose an option", (Item*)main_items, N(main_items), 0, &msel);
        if (a == A_PLAY) { result = 1; break; }
        if (a == A_EXIT) { result = 0; break; }
        if (a == A_AV) { if (run_menu("Audio / Video", av_items, N(av_items), 1, &sub) == A_EXIT) { result = 0; goto done; } }
        if (a == A_GAME) { if (run_menu("Gameplay", game_items, N(game_items), 0, &sub) == A_EXIT) { result = 0; goto done; } }
    }
done:
    if (memcmp(&start, &g_cfg, sizeof start) != 0) {
        if (cfg_save(cfg_path) != 0) fprintf(stderr, "launcher: could not save %s\n", cfg_path);
    }
    return result;
}
