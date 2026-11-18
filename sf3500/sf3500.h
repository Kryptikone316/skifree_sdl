/* SF3500 glue for the SkiFree SDL port: software renderer on an owned RGB565
 * surface, frames to the device driver (hwdisp), gamepad (joykey), test hooks.
 * Settings come from config.h (skifree.txt, SKI_* overrides). For off-device runs under QEMU: SKI_HEADLESS=1, SKI_SHOTS=<dir>,
 * SKI_SHOT_EVERY=<frames>, SKI_JOY="frame:mask,frame:mask", SKI_MAXFRAMES=<n>. */
#ifndef SF3500_GLUE_H
#define SF3500_GLUE_H

#include <SDL.h>

#define SF_W 640
#define SF_H 480

/* Button bits returned by sf3500_buttons() (same order as joykey.hpp's JK_*). */
enum { SB_LEFT = 1 << 0, SB_RIGHT = 1 << 1, SB_UP = 1 << 2, SB_DOWN = 1 << 3,
       SB_A = 1 << 4, SB_B = 1 << 5, SB_L1 = 1 << 6, SB_R1 = 1 << 7, SB_X = 1 << 8,
       SB_Y = 1 << 9, SB_START = 1 << 10, SB_SELECT = 1 << 11, SB_L2 = 1 << 12, SB_R2 = 1 << 13 };

/* Exit codes: 0 = leave the port (back to the front-end), SF_EXIT_MENU = back to the launcher
 * menu (run.sh starts the program again). */
#define SF_EXIT_MENU 10

int sf3500_start(void);                       /* display + gamepad; 0 = ok (call after cfg_load) */
unsigned short* sf3500_pixels(int* stride_px);   /* the 640x480 RGB565 screen (launcher drawing) */
void sf3500_set_fps(int fps);
void sf3500_mode_info(int mode, int* vw, int* vh, int* scale, int* ww, int* wh);
SDL_Renderer* sf3500_make_renderer(void);     /* software renderer on the 640x480 surface */
void sf3500_output_size(int* w, int* h);      /* playfield size in game pixels */
void sf3500_present(void);                    /* send the frame, cap the frame rate */
unsigned short sf3500_pal8(unsigned short rgb565);          /* nearest 8-bit-theme palette colour */
void sf3500_posterize(int x, int y, int w, int h);       /* snap a screen rectangle to that palette */
int sf3500_draw_ground(int scroll_x, int scroll_y);    /* 16-bit theme: paints the snow ground; 0 = theme off, clear instead */
unsigned sf3500_buttons(void);
void sf3500_quit(int code);                   /* release display, exit */
void sf3500_error_screen(const char* line1, const char* line2);

#endif
