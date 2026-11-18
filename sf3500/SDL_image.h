/* SF3500 build: the game only decodes BMP files (the bitmaps taken from ski32.exe),
 * which SDL itself can do, so SDL_image is not needed. */
#ifndef SF3500_SDL_IMAGE_SHIM_H
#define SF3500_SDL_IMAGE_SHIM_H
#include <SDL.h>
#define IMG_Load_RW(src, freesrc) SDL_LoadBMP_RW((src), (freesrc))
#endif
