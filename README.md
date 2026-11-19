# SkiFree for the Datafrog SF3500

This branch (`sf3500`) is a port of SkiFree to the Datafrog SF3500 handheld (MIPS32r2, no GPU). The game logic is
the original 1.04 as decompiled by Eric Fry, through jeff-1amstudios's SDL conversion. The port adds a launcher with
four themes (Classic, Modern 16-bit, Modern 8-bit, TI-92), sounds, sprite packs and the display, input and audio glue
for the device. See `NOTICE.md` for who made what and under which terms.

**The game's art is not included.** Players put their own `ski32.exe` (version 1.04, from https://ski.ihoc.net/) in
the port folder; the port reads the sprites from it at start.

## Building

```sh
./build_sf3500.sh            # skifree.stripped.mipsel in build-sf3500/
sf3500/package.sh            # the SD-card package and a zip, in dist/
```

You need the Codescape `mips-mti-linux-gnu` toolchain with SDL2 2.0.14 (`SF3500_TC=<toolchain root>`, see the header of
`build_sf3500.sh`), and for the package Python 3 with Pillow, numpy and scipy. The package script draws the sprite packs
and synthesises the sounds from `sf3500/tools/`, converts the launcher backdrop and copies the files in
`sf3500/package/`.

## Layout

| Path | What |
|--|--|
| `sf3500/launcher.c`, `theme16.c` | the launcher: menus, four themes, the 16-bit and 8-bit scenes |
| `sf3500/sf3500_glue.c`, `input.inc` | video modes, frame pacing, ground texture, palette filter, gamepad |
| `sf3500/sfsound.c`, `sf3500_mixer.cpp`, `hwaudio.cpp` | nine game sounds on a small software mixer |
| `sf3500/hwdisp.cpp`, `joykey.cpp` | the device's display driver and gamepad |
| `sf3500/peres.c` | reads the sprites from the player's `ski32.exe`; optional sprite packs |
| `sf3500/config.c` | `skifree.txt` |
| `sf3500/tools/` | scripts that draw the sprite packs and sounds |
| `sf3500/package/` | the files that ship as they are (run script, settings, READMEs) |
| `sf3500/assets/` | launcher backdrop and box art |

## Testing without the device

Under QEMU (`qemu-mipsel-static -L <sysroot> ./skifree.mipsel`) set `SKI_HEADLESS=1` to skip the display, `SKI_NOMENU=1`
to skip the launcher, `SKI_SHOTS=<dir>` with `SKI_SHOT_EVERY=<n>` to save frames, `SKI_JOY="frame:mask,..."` to script the
gamepad, `SKI_TRACE=<n>` to print the game state, and `SKI_LOOK`, `SKI_SCREEN`, `SKI_TIMING`, `SKI_YETI`, `SKI_SPEED`,
`SKI_SOUND` to override settings.

---

# Upstream README (desktop build)

Skifree SDL
=============================

A source port of the classic [Microsoft Entertainment Pack](https://en.wikipedia.org/wiki/Microsoft_Entertainment_Pack) game "skifree" to cross platform SDL2.

Forked from the decompiled skifree [skifree_decomp](https://github.com/yuv422/skifree_decomp) by Eric Fry

![Untitled](https://github.com/jeff-1amstudios/skifree_sdl/assets/1063652/95b53385-4d16-4de5-8f9d-10a877fee6a9)


## Dependencies
### Resources
You will need to place the original gfx and icon resources into `resources/`.
You can download the original version
from the [official Skifree website](https://ski.ihoc.net/) and extract the resources with a a PE resources extractor (e.g. [ResourcesExtract](https://www.nirsoft.net/utils/resources_extract.html))

Alternatively, you can fetch pre-extracted resources: 
```sh
curl -Lo /tmp/ski32_resources.zip https://archive.org/download/ski32_resources/ski32_resources.zip
unzip -d resources /tmp/ski32_resources.zip
```

### Libraries
- SDL2
- SDL2_image
- SDL2_ttf

## Compiling

This is a cmake project.

```sh
git clone https://github.com/jeff-1amstudios/skifree_sdl
cd skifree_sdl
# grab resources
curl -Lo /tmp/ski32_resources.zip https://archive.org/download/ski32_resources/ski32_resources.zip
unzip -d resources /tmp/ski32_resources.zip

mkdir build
cd build
cmake ..
```

Per-frame physics is enabled by default. To use the original 40 ms movement updates instead:

```sh
./build/skifree_sdl --no-per-frame-physics
```

On macOS, run the app bundle with the option using:

```sh
open build/skifree_sdl.app --args --no-per-frame-physics
```

## MacOS
On MacOS we build an app bundle `skifree_sdl.app`. Use right-click > Open the first time to get around [unverified developer warnings](https://support.apple.com/en-nz/guide/mac-help/mh40616/mac).

![Screenshot 2023-11-06 at 2 07 45 pm](https://github.com/jeff-1amstudios/skifree_sdl/assets/1063652/4edce399-ddeb-499a-a554-aebb7a70dfad)

## Todo
- Mouse support
- Sound maybe(?) - https://foone.wordpress.com/2017/06/20/uncovering-the-sounds-of-skifree/
