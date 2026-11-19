/* SF3500 build: a tiny stand-in for the few SDL_ttf calls SkiFree makes for its
 * status box (Time/Dist/Speed/Style). It draws with the public-domain font8x8
 * table, so neither SDL_ttf, FreeType nor Microsoft's vgaoem.fon are needed. */
#ifndef SF3500_SDL_TTF_SHIM_H
#define SF3500_SDL_TTF_SHIM_H

#include <SDL.h>

typedef struct TTF_Font TTF_Font;

int TTF_Init(void);
TTF_Font* TTF_OpenFontRW(SDL_RWops* src, int freesrc, int ptsize);
int TTF_SizeUTF8(TTF_Font* font, const char* text, int* w, int* h);
SDL_Surface* TTF_RenderUTF8_Blended_Wrapped(TTF_Font* font, const char* text, SDL_Color fg, Uint32 wrapLength);

/* Not part of SDL_ttf: draws text straight into an RGB565 buffer (error screen). */
void ttfshim_draw565(unsigned short* px, int stride, int w, int h, int x, int y, int scale,
                     unsigned short color, const char* text);
void ttfshim_draw565_px(unsigned short* px, int stride, int w, int h, int x, int y, int size,
                        unsigned short color, const char* text);   /* glyph cell of any pixel size */

#endif
