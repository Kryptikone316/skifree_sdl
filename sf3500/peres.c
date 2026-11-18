/* Reads the bitmaps of a 32-bit Windows executable (ski32.exe) without any
 * Windows API: walks the PE resource directory and wraps every RT_BITMAP in a
 * BMP file header, so SDL's normal BMP loader can decode it.
 * Written for this port (zlib-style terms like the rest of sf3500/). */
#include "embedded_resources.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_BITMAPS 128

static char g_err[160] = "";
static unsigned char* g_file;
static size_t g_size;
static embedded_resource_t g_res[MAX_BITMAPS];
static char g_names[MAX_BITMAPS][32];
static int g_count;

const char* peres_error(void) { return g_err; }
int peres_bitmap_count(void) { return g_count; }

static uint32_t rd32(size_t o) {
    return g_file[o] | (g_file[o + 1] << 8) | (g_file[o + 2] << 16) | ((uint32_t)g_file[o + 3] << 24);
}
static uint32_t rd16(size_t o) { return g_file[o] | (g_file[o + 1] << 8); }

static int fail(const char* msg) {
    snprintf(g_err, sizeof(g_err), "%s", msg);
    return -1;
}

typedef struct { uint32_t va, size, raw; } Section;
static Section g_sec[24];
static int g_nsec;

static int rva2off(uint32_t rva, size_t* off) {
    int i;
    for (i = 0; i < g_nsec; i++) {
        if (rva >= g_sec[i].va && rva < g_sec[i].va + g_sec[i].size) {
            *off = (size_t)g_sec[i].raw + (rva - g_sec[i].va);
            return *off < g_size ? 0 : -1;
        }
    }
    return -1;
}

int peres_open(const char* path) {
    FILE* f = fopen(path, "rb");
    uint32_t pe, rsrc_rva, opt;
    size_t base, n, off;
    int i, optsz, nsec;

    g_count = 0;
    if (!f) {
        snprintf(g_err, sizeof(g_err), "cannot open %s", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    g_size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    if (g_size < 1024 || g_size > (16u << 20)) {
        fclose(f);
        return fail("file size is not that of ski32.exe");
    }
    g_file = malloc(g_size);
    if (!g_file || fread(g_file, 1, g_size, f) != g_size) {
        fclose(f);
        return fail("read error");
    }
    fclose(f);

    if (g_file[0] != 'M' || g_file[1] != 'Z') return fail("not a Windows executable");
    pe = rd32(0x3c);
    if (pe + 256 > g_size || memcmp(g_file + pe, "PE\0\0", 4) != 0) return fail("not a PE file (a 16-bit SkiFree?)");
    nsec = (int)rd16(pe + 6);
    optsz = (int)rd16(pe + 20);
    opt = pe + 24;
    if (rd16(opt) != 0x10b) return fail("not a 32-bit PE file");
    if (nsec > 24) return fail("too many sections");
    for (i = 0; i < nsec; i++) {
        size_t s = opt + optsz + (size_t)i * 40;
        uint32_t vsz = rd32(s + 8), rsz = rd32(s + 16);
        g_sec[i].va = rd32(s + 12);
        g_sec[i].size = vsz > rsz ? vsz : rsz;
        g_sec[i].raw = rd32(s + 20);
    }
    g_nsec = nsec;
    rsrc_rva = rd32(opt + 96 + 16);
    if (!rsrc_rva || rva2off(rsrc_rva, &base) != 0) return fail("no resource section");

    /* Resource tree: type -> id -> language -> data entry. */
    {
        uint32_t nn = rd16(base + 12), ni = rd16(base + 14), t;
        for (t = 0; t < nn + ni; t++) {
            uint32_t tname = rd32(base + 16 + t * 8), ttgt = rd32(base + 20 + t * 8);
            size_t tdir, k;
            uint32_t tn2, ti2;
            if (tname != 2 || !(ttgt & 0x80000000u)) continue; /* RT_BITMAP only */
            tdir = base + (ttgt & 0x7fffffffu);
            tn2 = rd16(tdir + 12);
            ti2 = rd16(tdir + 14);
            for (k = 0; k < tn2 + ti2; k++) {
                uint32_t id = rd32(tdir + 16 + k * 8), itgt = rd32(tdir + 20 + k * 8);
                size_t idir, ldata;
                uint32_t ltgt, rva, size, hdr;
                int bpp;
                uint32_t clr, ncol, bmoff;
                unsigned char* out;
                if (!(itgt & 0x80000000u) || (id & 0x80000000u)) continue;
                idir = base + (itgt & 0x7fffffffu);
                ltgt = rd32(idir + 20); /* first language */
                if (ltgt & 0x80000000u) continue;
                ldata = base + ltgt;
                rva = rd32(ldata);
                size = rd32(ldata + 4);
                if (rva2off(rva, &off) != 0 || off + size > g_size || size < 40) continue;
                hdr = rd32(off);
                bpp = (int)rd16(off + 14);
                clr = rd32(off + 32);
                ncol = clr ? clr : (bpp <= 8 ? (1u << bpp) : 0);
                bmoff = 14 + hdr + ncol * 4;
                if (g_count >= MAX_BITMAPS) continue;
                out = malloc(14 + size);
                if (!out) return fail("out of memory");
                out[0] = 'B'; out[1] = 'M';
                n = 14 + size;
                out[2] = n & 255; out[3] = (n >> 8) & 255; out[4] = (n >> 16) & 255; out[5] = n >> 24;
                out[6] = out[7] = out[8] = out[9] = 0;
                out[10] = bmoff & 255; out[11] = (bmoff >> 8) & 255; out[12] = (bmoff >> 16) & 255; out[13] = bmoff >> 24;
                memcpy(out + 14, g_file + off, size);
                snprintf(g_names[g_count], sizeof(g_names[0]), "resources/ski32_%u.bmp", id);
                g_res[g_count].name = g_names[g_count];
                g_res[g_count].len = (int)(14 + size);
                g_res[g_count].content = (const char*)out;
                g_count++;
            }
        }
    }
    free(g_file);
    g_file = NULL;
    if (g_count != 89) {
        snprintf(g_err, sizeof(g_err), "found %d bitmaps, expected 89: this is not ski32.exe 1.04", g_count);
        return -1;
    }
    return 0;
}

static embedded_resource_t g_pack[90];   /* indexed by sprite id; content NULL = not replaced */

static int bmp_dims(const unsigned char* b, int len, int* w, int* h) {
    if (len < 26 || b[0] != 'B' || b[1] != 'M') return -1;
    *w = (int)(b[18] | (b[19] << 8) | (b[20] << 16) | (b[21] << 24));
    *h = (int)(b[22] | (b[23] << 8) | (b[24] << 16) | (b[25] << 24));
    if (*h < 0) *h = -*h;
    return 0;
}

int peres_use_pack(const char* dir) {
    int id, used = 0;
    for (id = 1; id < 90; id++) {
        char path[256], key[64];
        FILE* f;
        long size;
        unsigned char* buf;
        embedded_resource_t* orig;
        int w1, h1, w2, h2;
        snprintf(path, sizeof path, "%s/ski32_%d.bmp", dir, id);
        f = fopen(path, "rb");
        if (!f) continue;
        fseek(f, 0, SEEK_END); size = ftell(f); fseek(f, 0, SEEK_SET);
        buf = (size > 26 && size < 1024 * 1024) ? (unsigned char*)malloc((size_t)size) : NULL;
        if (!buf || fread(buf, 1, (size_t)size, f) != (size_t)size) { free(buf); fclose(f); continue; }
        fclose(f);
        snprintf(key, sizeof key, "resources/ski32_%d.bmp", id);
        orig = get_embedded_resource_by_name(key);
        if (!orig || bmp_dims((const unsigned char*)orig->content, orig->len, &w1, &h1) != 0 ||
            bmp_dims(buf, (int)size, &w2, &h2) != 0 || w1 != w2 || h1 != h2) {
            fprintf(stderr, "sprites: %s skipped (size differs from the exe's sprite)\n", path);
            free(buf);
            continue;
        }
        g_pack[id].name = "pack";
        g_pack[id].len = (int)size;
        g_pack[id].content = (const char*)buf;
        used++;
    }
    fprintf(stderr, "sprites: %d replaced from %s\n", used, dir);
    return used;
}

embedded_resource_t* get_embedded_resource_by_name(char* name) {
    static embedded_resource_t fon = { "resources/vgaoem.fon", 0, "" };
    int i, id;
    if (strcmp(name, fon.name) == 0) return &fon;
    if (sscanf(name, "resources/ski32_%d.bmp", &id) == 1 && id > 0 && id < 90 && g_pack[id].content) return &g_pack[id];
    for (i = 0; i < g_count; i++)
        if (strcmp(name, g_res[i].name) == 0) return &g_res[i];
    return NULL;
}
