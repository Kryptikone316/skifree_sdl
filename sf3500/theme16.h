/* The "16-bit" launcher theme's backdrop, drawn in code (original pixel art, no image files):
 * a snowy mountain scene with pines, a ski lift, drifts, the skier and the yeti. Painted once
 * into a 640x480 RGB565 buffer on a 320x240 grid, so the pixels are chunky 2x2. */
#ifndef SF3500_THEME16_H
#define SF3500_THEME16_H

void theme16_render(unsigned short* dst, int stride_px, int variant);   /* dst: 640x480 RGB565; variant 1 = the 8-bit look */

#endif
