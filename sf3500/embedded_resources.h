/* SF3500 build: replaces the build-time generated embedded_resources.h of the
 * desktop build. The game's artwork is not compiled in; it is read from the
 * player's own ski32.exe when the game starts (see peres.c). The names are the
 * ones the game already asks for: "resources/ski32_<n>.bmp". */
#ifndef SF3500_EMBEDDED_RESOURCES_H
#define SF3500_EMBEDDED_RESOURCES_H

typedef struct embedded_resource_t {
    const char* name;
    int len;
    const char* content;
} embedded_resource_t;

/* Loads the PE resources of the given ski32.exe. Returns 0 on success, or a
 * negative number (message in peres_error()). */
int peres_open(const char* exe_path);
const char* peres_error(void);
int peres_bitmap_count(void);

/* Optional replacement sprites: <dir>/ski32_<id>.bmp. A file is used only when its width and height
 * equal the exe's sprite (the sizes drive the game's collisions); the rest stay as they are.
 * Returns the number of sprites replaced. */
int peres_use_pack(const char* dir);

/* Returns NULL when the resource does not exist. "resources/vgaoem.fon" is
 * answered with a dummy entry: the font shim does not read it. */
embedded_resource_t* get_embedded_resource_by_name(char* name);

#endif
