#include "sfsound.h"
#include "config.h"
#include "sf3500_mixer.hpp"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NSLOTS 9
#define OUT_RATE 48000
#define MAX_SECONDS 8

static const char* const k_names[NSLOTS] = {
    "crash", "jump", "woof", "land", "board", "bump", "gobble", "piddle", "roar"
};

typedef struct { short* pcm; int frames; Sf3500Voice* voice; } Slot;
static Slot g_slot[NSLOTS];
static int g_loaded_custom = -1, g_ready, g_failed, g_log = -1;

const char* sfsnd_slot_name(int slot) {
    return (slot >= 1 && slot <= NSLOTS) ? k_names[slot - 1] : "";
}

static unsigned rd16(const unsigned char* p) { return p[0] | (p[1] << 8); }
static unsigned rd32(const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24); }

/* Reads a PCM .wav (8 or 16 bit, mono or stereo, any rate) into 48 kHz stereo S16. */
static int load_wav(const char* path, short** out, int* out_frames) {
    FILE* f = fopen(path, "rb");
    unsigned char* buf;
    long size;
    long pos;
    int channels = 0, bits = 0, tag = 0;
    unsigned rate = 0;
    const unsigned char* data = NULL;
    unsigned data_len = 0;
    int in_frames, n, i;
    short* pcm;

    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 44 || size > 16 * 1024 * 1024) { fclose(f); return -2; }
    buf = (unsigned char*)malloc((size_t)size);
    if (!buf || fread(buf, 1, (size_t)size, f) != (size_t)size) { free(buf); fclose(f); return -2; }
    fclose(f);
    if (memcmp(buf, "RIFF", 4) || memcmp(buf + 8, "WAVE", 4)) { free(buf); return -3; }
    for (pos = 12; pos + 8 <= size;) {
        unsigned len = rd32(buf + pos + 4);
        const unsigned char* body = buf + pos + 8;
        if (!memcmp(buf + pos, "fmt ", 4) && len >= 16 && pos + 8 + 16 <= size) {
            tag = (int)rd16(body); channels = (int)rd16(body + 2);
            rate = rd32(body + 4); bits = (int)rd16(body + 14);
        } else if (!memcmp(buf + pos, "data", 4)) {
            data = body;
            data_len = len;
            if ((long)(pos + 8 + len) > size) data_len = (unsigned)(size - pos - 8);
            break;
        }
        pos += 8 + (long)len + (len & 1);
    }
    if (tag != 1 || (bits != 8 && bits != 16) || (channels != 1 && channels != 2) || rate < 4000 || rate > 192000 || !data) {
        free(buf);
        return -4;
    }
    in_frames = (int)(data_len / (unsigned)(channels * bits / 8));
    n = (int)((long long)in_frames * OUT_RATE / (long long)rate);
    if (n < 1) { free(buf); return -4; }
    if (n > OUT_RATE * MAX_SECONDS) n = OUT_RATE * MAX_SECONDS;
    pcm = (short*)malloc((size_t)n * 2 * sizeof(short));
    if (!pcm) { free(buf); return -2; }
    for (i = 0; i < n; i++) {
        double src = (double)i * (double)rate / OUT_RATE;
        int i0 = (int)src, i1 = i0 + 1, c;
        double fr = src - i0;
        double v[2];
        if (i1 >= in_frames) i1 = in_frames - 1;
        if (i0 >= in_frames) i0 = in_frames - 1;
        for (c = 0; c < channels; c++) {
            double a, b;
            if (bits == 16) {
                a = (short)rd16(data + (i0 * channels + c) * 2);
                b = (short)rd16(data + (i1 * channels + c) * 2);
            } else {
                a = ((int)data[i0 * channels + c] - 128) * 256;
                b = ((int)data[i1 * channels + c] - 128) * 256;
            }
            v[c] = a + (b - a) * fr;
        }
        if (channels == 1) v[1] = v[0];
        pcm[i * 2] = (short)v[0];
        pcm[i * 2 + 1] = (short)v[1];
    }
    free(buf);
    *out = pcm;
    *out_frames = n;
    return 0;
}

static int file_exists(const char* p) {
    FILE* f = fopen(p, "rb");
    if (f) fclose(f);
    return f != NULL;
}

int sfsnd_custom_count(void) {
    char path[160];
    int i, n = 0;
    for (i = 0; i < NSLOTS; i++) {
        snprintf(path, sizeof path, "sounds/custom/%s.wav", k_names[i]);
        n += file_exists(path);
    }
    return n;
}

static void unload(void) {
    int i;
    for (i = 0; i < NSLOTS; i++) {
        if (g_slot[i].voice) sf3500_voice_destroy(g_slot[i].voice);
        free(g_slot[i].pcm);
        memset(&g_slot[i], 0, sizeof g_slot[i]);
    }
}

/* (Re)loads the nine sounds for the current "custom" setting. */
static void load_all(void) {
    char path[160];
    int i, custom_used = 0, builtin_used = 0;
    unload();
    for (i = 0; i < NSLOTS; i++) {
        short* pcm = NULL;
        int frames = 0, rc = -1, from_custom = 0;
        if (g_cfg.custom) {
            snprintf(path, sizeof path, "sounds/custom/%s.wav", k_names[i]);
            rc = load_wav(path, &pcm, &frames);
            if (rc == 0) from_custom = 1;
            else if (rc != -1) fprintf(stderr, "sound: %s unreadable (%d), using the built-in sound\n", path, rc);
        }
        if (rc != 0) {
            { char rel[64]; snprintf(rel, sizeof rel, "sounds/%s.wav", k_names[i]); cfg_asset_path(path, sizeof path, rel); }
            rc = load_wav(path, &pcm, &frames);
            if (rc != 0) { fprintf(stderr, "sound: %s missing or unreadable (%d)\n", path, rc); continue; }
        }
        from_custom ? custom_used++ : builtin_used++;
        g_slot[i].pcm = pcm;
        g_slot[i].frames = frames;
        g_slot[i].voice = sf3500_voice_create_static(pcm, frames);
        if (g_slot[i].voice) sf3500_voice_set_relative(g_slot[i].voice, true);
    }
    g_loaded_custom = g_cfg.custom;
    fprintf(stderr, "sound: loaded %d built-in, %d custom\n", builtin_used, custom_used);
}

static int ensure(void) {
    if (g_log < 0) g_log = getenv("SKI_SNDLOG") != NULL;
    if (!g_cfg.sound) return 0;
    if (g_failed) return 0;
    if (!g_ready) {
        if (!(getenv("SKI_HEADLESS") && atoi(getenv("SKI_HEADLESS")))) {
            if (!sf3500_mixer_init()) {
                fprintf(stderr, "sound: mixer/hwaudio unavailable, sound off\n");
                g_failed = 1;
                return 0;
            }
        }
        g_ready = 1;
        g_loaded_custom = -1;
    }
    if (g_loaded_custom != g_cfg.custom) load_all();
    return 1;
}

void sfsnd_prepare(void) { ensure(); }

static float gain_for(int volume) {
    float v = volume / 10.0f;
    return v * v;
}

static void play_slot(int i) {
    static unsigned last_roar;
    unsigned now = SDL_GetTicks();
    if (!ensure() || g_cfg.volume <= 0) return;
    if (!g_slot[i].voice) return;
    /* the yeti calls its sound on every step of a chase; let a roar finish first */
    if (i == 8) {
        if (sf3500_voice_playing(g_slot[i].voice) || (last_roar && now - last_roar < 1500)) return;
        last_roar = now;
    }
    if (g_log) fprintf(stderr, "snd: %s vol=%d frames=%d\n", k_names[i], g_cfg.volume, g_slot[i].frames);
    sf3500_voice_set_gain(g_slot[i].voice, gain_for(g_cfg.volume));
    sf3500_voice_play(g_slot[i].voice);
}

void sfsnd_play(int slot) {
    if (slot >= 1 && slot <= NSLOTS) play_slot(slot - 1);
}

void sfsnd_play_by_name(const char* name) {
    int i;
    for (i = 0; i < NSLOTS; i++)
        if (!strcmp(name, k_names[i])) { play_slot(i); return; }
}

void sfsnd_shutdown(void) {
    if (g_ready) {
        unload();
        if (!(getenv("SKI_HEADLESS") && atoi(getenv("SKI_HEADLESS")))) sf3500_mixer_deinit();
        g_ready = 0;
    }
}
