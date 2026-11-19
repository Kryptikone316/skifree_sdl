# SkiFree for the Datafrog SF3500

This branch (`sf3500`) is jeff-1amstudios's `skifree_sdl` (an SDL conversion of Eric Fry's
[skifree_decomp](https://github.com/yuv422/skifree_decomp), the byte-compatible decompilation of SkiFree 1.04),
commit `e42f2e8`, plus a port to the Datafrog SF3500 handheld (MIPS32r2, no GPU). The game's art is not in this
repository or in the port package: the player brings their own `ski32.exe` (version 1.04, from
https://ski.ihoc.net/) and the port reads the sprites from it when it starts.

The commits on top of upstream are:

1. **SF3500 platform backend** (`sf3500/` and the hooks in `src/skifree_decomp.c`, all under `#ifdef SF3500`):
   the game renders with SDL's software renderer into an owned 640x480 RGB565 surface that is handed to the device's
   display driver; the gamepad becomes the game's keys and mouse click (`sf3500/input.inc`); the sprites are read
   from the player's `ski32.exe` (`sf3500/peres.c`, a small PE resource reader, in place of the build-time embedded
   resources); the status text uses the public-domain font8x8 (`sf3500/ttfshim.c`, in place of SDL_ttf and
   Microsoft's font); SDL_image is replaced by `SDL_LoadBMP_RW`. The driver comes from `/tmp/tfdevice.env`.
2. **A fix to the fork's per-frame physics**: the course-start checks now run on every movement step, so Slalom and
   Tree Slalom start (the clock used to stay at 0).
3. **A launcher and settings**: Play Game, How To Play, Audio / Video, Gameplay and Exit, with four themes (Classic,
   Modern 16-bit, Modern 8-bit, TI-92). Options for sound and volume, screen size, frame rate, frame timing (smooth
   or the original 40 ms steps), yeti distance and game speed. Settings live in `skifree.txt`.
4. **Sound**: nine game sounds on a small software mixer (`sf3500/sfsound.c`, `sf3500_mixer.cpp`), written to the
   device's audio driver.
5. **Sprite packs and generators**: optional 16-bit and 8-bit sprite packs, the built-in sounds and the launcher
   art, with the scripts that make them in `sf3500/tools/`, and `sf3500/package.sh` to assemble the SD-card package.

`NOTICE.md` says who made what and under which terms.

## Package layout

```text
roms/ports/skifree/
├── skifree.mipsel, run.sh        the program and its launch script
├── launcher.bmp, sounds/, sprites16/, sprites8/    the port's own files
├── README.txt, licenses/       credits and the licence for the assets
└── gamedata/                     what the player supplies or the game creates
    ├── ski32.exe                 the game (supplied by the player)
    ├── skifree.txt               settings, written by the launcher
    └── sounds/custom/            the player's own replacement sounds
```

`run.sh` runs the program from `gamedata/` and sets `SKI_ASSETS` to the port folder, which is where the program finds
`launcher.bmp`, `sounds/`, `sprites16/` and `sprites8/`.

## Building

```sh
./build_sf3500.sh            # build-sf3500/skifree.stripped.mipsel
sf3500/package.sh            # the SD-card package and a zip, in dist/
```

The toolchain is a MIPS32r2 little-endian GCC (tested with Codescape MIPS MTI GCC 6.3.0) whose sysroot has the SDL2
headers and libraries (`SF3500_TC=<toolchain root>`, see the header of `build_sf3500.sh`). The binary is linked
against the device's SDL2 and runs with SDL's dummy video and audio drivers, so no SDL patch is needed. The package
script also needs Python 3 with Pillow, numpy and scipy.

## Testing without the device

Under QEMU (`qemu-mipsel-static -L <sysroot> ./skifree.mipsel`, started in a folder that holds `ski32.exe`) set
`SKI_HEADLESS=1` to skip the display, `SKI_NOMENU=1` to skip the launcher, `SKI_SHOTS=<dir>` with
`SKI_SHOT_EVERY=<n>` to save frames, `SKI_JOY="frame:mask,..."` to script the gamepad, `SKI_TRACE=<n>` to print the
game state, and `SKI_LOOK`, `SKI_SCREEN`, `SKI_TIMING`, `SKI_YETI`, `SKI_SPEED`, `SKI_SOUND` to override settings.
`SKI_EXE` names the game file and `SKI_ASSETS` the folder with the port's own files.

## Licence

See `NOTICE.md`. Upstream states no licence; the assets made for this port are MIT (`sf3500/LICENSE-ASSETS`).
