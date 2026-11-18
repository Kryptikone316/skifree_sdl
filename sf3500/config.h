/* SkiFree launcher settings. Kept in skifree.txt as plain "name=number" lines,
 * read at start and rewritten by the launcher menu. Environment variables
 * (SKI_*) override the file for off-device tests. */
#ifndef SF3500_CONFIG_H
#define SF3500_CONFIG_H

#define CFG_NMODES 5

typedef struct {
    int sound;    /* 1 = sounds on, 0 = off (nothing is loaded, custom sounds included) */
    int volume;   /* 0 (mute) .. 10 */
    int custom;   /* 1 = use sounds/custom/<name>.wav where present, 0 = built-in set only */
    int mode;     /* video mode index, see sf3500_glue.c */
    int fps;      /* frame rate cap */
    int timing;   /* 0 = smooth (physics every frame), 1 = original (40 ms steps) */
    int yeti;     /* metres before the yeti shows up, 0 = never */
    int speed;    /* game speed in percent, 100 = original */
    int nice;     /* process priority */
} SkiCfg;

extern SkiCfg g_cfg;

void cfg_defaults(void);
void cfg_load(const char* path);   /* file, then SKI_* overrides */
int cfg_save(const char* path);    /* 0 = ok */

#endif
