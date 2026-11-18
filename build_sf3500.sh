#!/bin/bash
# Cross build for the Datafrog SF3500 (MIPS32r2, little endian). Output: skifree.mipsel
# Needs the toolchain from ~/handhelds/sf3000-work/buildroot (Codescape mips-mti-linux-gnu, glibc 2.20 sysroot
# with SDL2 2.0.14). Override with SF3500_TC=<buildroot/output/host>.
set -e
cd "$(dirname "$0")"
TC=${SF3500_TC:-$HOME/handhelds/sf3000-work/buildroot/output/host}
SYSROOT=$TC/mipsel-buildroot-linux-gnu/sysroot
CC=$TC/bin/mips-mti-linux-gnu-gcc
CXX=$TC/bin/mips-mti-linux-gnu-g++
ARCH="-mips32r2 -mtune=74kc -mhard-float -EL --sysroot=$SYSROOT"
# -O2 on purpose (see the build-optimisation lesson); no fast-math, no dspr2
CFLAGS="$ARCH -O2 -fno-strict-aliasing -w -DSF3500 -Isf3500 -I$SYSROOT/usr/include/SDL2"
CXXFLAGS="$ARCH -O2 -w -fno-exceptions -fno-rtti -Isf3500"
OUT=${OUT:-build-sf3500}
mkdir -p "$OUT"
$CC $CFLAGS -c src/skifree_decomp.c -o "$OUT/skifree_decomp.o"
for f in peres ttfshim sf3500_glue config sfsound launcher theme16; do $CC $CFLAGS -c sf3500/$f.c -o "$OUT/$f.o"; done
for f in hwdisp joykey hwaudio sf3500_mixer; do $CXX $CXXFLAGS -c sf3500/$f.cpp -o "$OUT/$f.o"; done
$CXX $ARCH "$OUT"/*.o -o "$OUT/skifree.mipsel" -lSDL2 -lpthread -ldl -lm
$TC/bin/mips-mti-linux-gnu-strip -o "$OUT/skifree.stripped.mipsel" "$OUT/skifree.mipsel"
ls -l "$OUT"/skifree*.mipsel
