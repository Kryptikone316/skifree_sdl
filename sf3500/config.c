#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SkiCfg g_cfg;

void cfg_defaults(void) {
    g_cfg.sound = 1;
    g_cfg.volume = 8;
    g_cfg.custom = 1;
    g_cfg.mode = 0;
    g_cfg.fps = 40;
    g_cfg.timing = 1;
    g_cfg.yeti = 2000;
    g_cfg.speed = 100;
    g_cfg.look = 0;
    g_cfg.nice = -10;
}

char* cfg_asset_path(char* buf, unsigned n, const char* rel) {
    const char* dir = getenv("SKI_ASSETS");
    if (dir && *dir) snprintf(buf, n, "%s/%s", dir, rel);
    else snprintf(buf, n, "%s", rel);
    return buf;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void validate(void) {
    g_cfg.sound = g_cfg.sound != 0;
    g_cfg.volume = clampi(g_cfg.volume, 0, 10);
    g_cfg.custom = g_cfg.custom != 0;
    g_cfg.mode = clampi(g_cfg.mode, 0, CFG_NMODES - 1);
    g_cfg.fps = clampi(g_cfg.fps, 10, 120);
    g_cfg.timing = g_cfg.timing != 0;
    g_cfg.yeti = clampi(g_cfg.yeti, 0, 2000);   /* the skier's y position is a 16-bit number: 2047 m is the end of the map */
    g_cfg.speed = clampi(g_cfg.speed, 25, 200);
    g_cfg.look = clampi(g_cfg.look, 0, 3);
    g_cfg.nice = clampi(g_cfg.nice, -20, 19);
}

static void set(const char* key, int v) {
    if (!strcmp(key, "sound")) g_cfg.sound = v;
    else if (!strcmp(key, "volume")) g_cfg.volume = v;
    else if (!strcmp(key, "custom_sounds")) g_cfg.custom = v;
    else if (!strcmp(key, "screen")) g_cfg.mode = v;
    else if (!strcmp(key, "fps")) g_cfg.fps = v;
    else if (!strcmp(key, "timing")) g_cfg.timing = v;
    else if (!strcmp(key, "yeti")) g_cfg.yeti = v;
    else if (!strcmp(key, "speed")) g_cfg.speed = v;
    else if (!strcmp(key, "look")) g_cfg.look = v;
    else if (!strcmp(key, "nice")) g_cfg.nice = v;
}

static int env_has(const char* n) { const char* v = getenv(n); return v && *v; }

void cfg_load(const char* path) {
    FILE* f;
    char line[256];
    cfg_defaults();
    f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof line, f)) {
            char key[64];
            char* eq;
            char* p = line;
            size_t n = 0;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '#' || *p == '\n' || *p == '\r' || !*p) continue;
            eq = strchr(p, '=');
            if (!eq) continue;
            while (p < eq && n < sizeof key - 1) {
                if (*p != ' ' && *p != '\t') key[n++] = *p;
                p++;
            }
            key[n] = 0;
            set(key, atoi(eq + 1));
        }
        fclose(f);
    }
    /* test/override environment */
    if (env_has("SKI_ZOOM")) g_cfg.mode = atoi(getenv("SKI_ZOOM")) >= 2 ? 3 : 0;
    if (env_has("SKI_SCREEN")) g_cfg.mode = atoi(getenv("SKI_SCREEN"));
    if (env_has("SKI_FPS")) g_cfg.fps = atoi(getenv("SKI_FPS"));
    if (env_has("SKI_NICE")) g_cfg.nice = atoi(getenv("SKI_NICE"));
    if (env_has("SKI_SOUND")) g_cfg.sound = atoi(getenv("SKI_SOUND"));
    if (env_has("SKI_VOLUME")) g_cfg.volume = atoi(getenv("SKI_VOLUME"));
    if (env_has("SKI_CUSTOM")) g_cfg.custom = atoi(getenv("SKI_CUSTOM"));
    if (env_has("SKI_TIMING")) g_cfg.timing = atoi(getenv("SKI_TIMING"));
    if (env_has("SKI_YETI")) g_cfg.yeti = atoi(getenv("SKI_YETI"));
    if (env_has("SKI_SPEED")) g_cfg.speed = atoi(getenv("SKI_SPEED"));
    if (env_has("SKI_LOOK")) g_cfg.look = atoi(getenv("SKI_LOOK"));
    validate();
}

int cfg_save(const char* path) {
    char tmp[600];
    FILE* f;
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "w");
    if (!f) return -1;
    fprintf(f,
        "# SkiFree settings. The launcher menu writes this file; you can also edit it.\n"
        "# Plain name=number lines.\n"
        "#\n"
        "# sound: 1 = on, 0 = off (also ignores custom sounds)\n"
        "sound=%d\n"
        "# volume: 0 (mute) to 10\n"
        "volume=%d\n"
        "# custom_sounds: 1 = use files from sounds/custom/ where present, 0 = built-in sounds only\n"
        "custom_sounds=%d\n"
        "# screen: 0 = 640x480 full, 1 = 480x360 window, 2 = 320x240 window,\n"
        "#         3 = 2x zoom full screen (320x240 view), 4 = 2x zoom 512x384 window (256x192 view)\n"
        "screen=%d\n"
        "# fps: frame rate cap (the game keeps its speed whatever the frame rate)\n"
        "fps=%d\n"
        "# timing: 0 = smooth (the game moves every frame), 1 = original (40 ms steps, like Windows)\n"
        "timing=%d\n"
        "# yeti: metres before the yeti appears (2000 = original), 0 = no yeti\n"
        "yeti=%d\n"
        "# speed: game speed in percent (100 = original)\n"
        "speed=%d\n"
        "# look: the theme, switched with L1/R1 in the launcher menu. 0 = Classic (Windows 3.x menus, original colours),\n"
        "#        1 = TI-92 calculator menus and game (four LCD grays, colours only, the game is unchanged),\n"
        "#        2 = 16-bit menus and game art, 3 = the same in 8-bit (half-resolution art, one fixed palette)\n"
        "look=%d\n"
        "# nice: process priority, -10 keeps the frames even\n"
        "nice=%d\n",
        g_cfg.sound, g_cfg.volume, g_cfg.custom, g_cfg.mode, g_cfg.fps, g_cfg.timing,
        g_cfg.yeti, g_cfg.speed, g_cfg.look, g_cfg.nice);
    if (fclose(f) != 0) { remove(tmp); return -1; }
    return rename(tmp, path);
}
